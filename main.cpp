#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <tuple>
#include <semaphore.h>
#include <algorithm>

using namespace std;

// element kolejki dwukierunkowej
struct node
{
	int val, thread; 	// wartość ścieżki i nr wątku, który ją dorzucił
	struct node *prev; 	// wskaźnik na poprzedni element kolejki
	struct node *next;	// wskaźnik na kolejny element kolejki
};

/* Kolejka SSTF oparta na kolejce dwukierunkowej
Ogólny pomysł polegał na stworzeniu uporządkowanej kolejki składającej się z elementów potrzebujących dostępu do dysku
oraz elementu sybolizującego aktualne położenie głowicy na dysku.
*/
class SSTF_Queue
{
	node *first, *last, *curr;	// wskaźniki na pierwszy i ostatni element oraz wskaźnik na element symbolizujący położenie głowicy

public:
	int capacity, curr_size;	// pojemność i aktualny rozmiar kolejki

	SSTF_Queue(int capacity) : first{new node}, last{new node}, curr{new node}, capacity{capacity}, curr_size{0}
	{
		first->val = 0;
		first->thread = -1;
		first->prev = NULL;
		first->next = NULL;
		curr = first;
		last = first;
	}

	void insert(int val, int thread)	// dodawanie elementu do kolejki w odpowieniej kolejności
	{
		node *temp, *ptr1, *ptr2;

		temp = new node;
		temp->val = val;
		temp->thread = thread;

		if (curr_size < capacity)
		{
			if (val < first->val)
			{
				temp->next = first;
				first->prev = temp;
				first = temp;
			}
			else if (val > last->val)
			{
				temp->prev = last;
				last->next = temp;
				last = temp;
			}
			else
			{
				ptr1 = first;
				ptr2 = first->next;

				while (ptr2->next != NULL && ptr1->next->val <= val)
				{
					ptr1 = ptr1->next;
					ptr2 = ptr2->next;
				}

				//tu jest coś nie tak
				temp->next = ptr2;
				ptr2->prev = temp;
				temp->prev = ptr1;
				ptr1->next = temp;
			}
			curr_size++;
		}
	}

	tuple<int, int> take()	// zabieranie elementu z kolejki. Zwracana jest krotka zawierająca wartość ścieżki
							// i nr wątku, który ją dodał
	{
		int ret = -1;
		int curr_val, thread;
		node *temp = curr;

		curr_val = curr->val;

		if (curr_size == 0)
		{
			cout << "Kolejka pusta";
		}
		else
		{
			if (curr == first)
			{
				curr = curr->next;
				curr->prev = NULL;
				first = curr;
				ret = curr->val;
				thread = curr->thread;
			}
			else if (curr == last)
			{
				curr = curr->prev;
				curr->next = NULL;
				last = curr;
				ret = curr->val;
				thread = curr->thread;
			}
			else if (curr_val - curr->prev->val < curr->next->val - curr_val)
			{
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
				curr = curr->prev;
				ret = curr->val;
				thread = curr->thread;
			}
			else
			{
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
				curr = curr->next;
				ret = curr->val;
				thread = curr->thread;
			}
			free(temp);
			curr_size--;
		}
		return make_tuple(ret, thread);
	}
};



int max_queue_capacity;			// maksymalna pojemność kolejki
int num_liv_thr;				// liczba wciąż żyjących wątków

sem_t *Semaphores;				// tablica semaforów kontrolujących, czy dany wątek ma już ścieżkę w kolejce
sem_t full_queue_sem;			// semafor kontrolujący, czy kolejka może być bardziej pełna, aby zminimalizować średni czas dostępu
sem_t max_queue_capacity_sem;	// semafor kontrolujący, czy kolejka nie przekracza swojej maksymalnej wielkości
sem_t queue_mut;				// semafor ograniczający dostęp do kolejki tylko dla jednego wątku w celu wykonania operacji na niej

mutex print;					// muteks blokujący dostęp do wyjścia
mutex num_liv_thr_mut;			// muteks blokujący dostęp do zmiennej num_liv_thr

// obsługa konkretnego wątku: odczyt ścieżek z pliku, dodawanie ścieżek do kolejki
void thread_handling(string name, int num, SSTF_Queue *queue)
{
	fstream file;
	file.open(name, ios::in);		// otwieranie pliku
	if (file.good())
	{
		int line = 0;
		string track;
		while (!file.eof())
		{
			getline(file, track);				// wczytanie wartości ścieżki
			sem_wait(&Semaphores[num]);			// wątek czeka, aż jego wcześniejsze żądanie zostanie obsłużone
			sem_wait(&max_queue_capacity_sem);	// wątek czeka, aż kolejka nie będzie pełna
			sem_wait(&queue_mut);				// wątek "rezerwuje" kolejkę dla siebie
				print.lock();					// blokada wyjścia
					queue->insert(atoi(track.c_str()), num);	//dodawanie ścieżki do kolejki
					cout << "requester " << num << " track " << atoi(track.c_str()) << endl;	// wypisanie odpowiedniego komunikatu
				print.unlock();					// odblokowanie wyjścia
				num_liv_thr_mut.lock();			// blokowanie zmiennej
					if (queue->curr_size == min(max_queue_capacity, num_liv_thr)){	// jeśli kolejka nie może być bardziej pełna,
						sem_post(&full_queue_sem);									// to możemy zabrać z niej element
					}
				num_liv_thr_mut.unlock();		// odblokowanie zmiennej
			sem_post(&queue_mut);				// udostępnienie kolejki innym wątkom
			line++;
		}
	}
	else cout << "Błąd dostępu do pliku" << endl;

	sem_wait(&Semaphores[num]);					// wątek czeka, aż jego ostatnie żądanie wyjdzie z kolejki
	sem_wait(&queue_mut);
	num_liv_thr_mut.lock();						// blokada operacji na zmiennej
		num_liv_thr--;							// zmniejszenie liczby żywych wątków (ten już nic nie doda)
		if (queue->curr_size == num_liv_thr) {	// jeśli liczba wątków w kolejce jest równa liczbie żywych wątków,
			sem_post(&full_queue_sem);			// kolejka nie będzie pełna bardziej
		}
	num_liv_thr_mut.unlock();					// odblokowanie operacji na zmiennej
	sem_post(&queue_mut);
}

// wątek zarządzający
void main_thread(SSTF_Queue *queue, int num)
{
	tuple<int, int> tuple;
	
	while (queue->curr_size > 0 || num_liv_thr > 0)
	{
		sem_wait(&full_queue_sem);				// zarządca czeka, aż nie będzie możliwe dodanie do kolejki nowego elementu

		num_liv_thr_mut.lock();
			if (num_liv_thr == 0) {				// jeśli liczba żywych wątków jest równa 0, to wątek się kończy
				break;
			}
		num_liv_thr_mut.unlock();

		sem_wait(&queue_mut);					// blokada operacji na kolejce dla innych wątków
			print.lock();						// blokada wyjścia
				tuple = queue->take();			// pobranie elementu z kolejki
				cout << "service requester " << get<1>(tuple) << " thread " << get<0>(tuple) << endl;	// wypisanie odpowiedniego komunikatu
			print.unlock();						// odblokowanie wyjścia
		sem_post(&queue_mut);					// odblokowanie operacji na kolejce dla innych wątków
		sem_post(&max_queue_capacity_sem);		// kolejka nie jest już pełna
		sem_post(&Semaphores[get<1>(tuple)]);	// dany wątek może dodać nowe żądanie do kolejki
	}
}

int main(int argc, char const *argv[])
{
	// przypisanie odpowiednich wartości zmiennym
	int threads_num = argc-2;
	num_liv_thr = argc-2;
	max_queue_capacity = atoi(argv[1]);

	thread Threads[threads_num+1];										// tablica wątków wraz z wątkiem zarządzającym
	Semaphores = new sem_t[threads_num];								// tablica semaforów
	SSTF_Queue queue = SSTF_Queue(max_queue_capacity);					// utworzenie kolejki o włąściwej pojemności

	// inicjalizacja semaforów
	sem_init(&full_queue_sem, 0, 0);
	sem_init(&max_queue_capacity_sem, 0, max_queue_capacity);
	sem_init(&queue_mut, 0, 1);

	for (int i = 0; i < threads_num; i++)
	{
		sem_init(&Semaphores[i], 0, 1);									// inicjalizacja semaforów
		Threads[i] = thread(&thread_handling, argv[i+2], i, &queue);	// tworzenie wątków
	}
	Threads[threads_num] = thread(&main_thread, &queue, threads_num);	// tworzenie wątku zarządzającego
	for (int i = 0; i < argc-1; i++)
	{
		Threads[i].join();												// zakończenie wątków
	}
	return 0;
}