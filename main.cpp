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

	void insert(int val, int thread)
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

	tuple<int, int> take()
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

	void print()
	{
		node *pom;
		pom = first;

		cout << first->val << ", " << curr->val <<": ";

		if (first == NULL) 
		{
			cout << "Queue is empty\n";
		}
		else
		{
			while (pom != curr)
			{
				cout << pom->val << " ";
				pom = pom->next;
			}
			cout << " *" << pom->val << " ";
			pom = pom->next;
			while (pom != NULL)
			{
				cout << pom->val << " ";
				pom = pom->next;
			}
			cout << endl;
		}
	}
};

int max_queue_capacity;
int num_liv_thr;

sem_t *Semaphores;
sem_t full_queue_sem;
sem_t max_queue_capacity_sem;
sem_t queue_mut;

mutex print;
mutex num_liv_thr_mut;

void thread_handling(string name, int num, SSTF_Queue *queue)
{
	vector<int> tracks;
	fstream file;
	file.open(name, ios::in);
	if (file.good())
	{
		int line = 0;
		string track;
		while (!file.eof())
		{
			getline(file, track);
			tracks.push_back(atoi(track.c_str()));
			line++;
		}
	}
	else cout << "Błąd dostępu do pliku" << endl;

	//cout << tracks.size() << endl;
	for (size_t i = 0; i < tracks.size(); i++)
	{
		//cout << i << endl;
		sem_wait(&Semaphores[num]);
		sem_wait(&max_queue_capacity_sem);
		sem_wait(&queue_mut);
			print.lock();
				queue->insert(tracks[i], num);
				cout << "requester " << num << " track " << tracks[i] << endl;
			print.unlock();
			num_liv_thr_mut.lock();
			//cout << queue->curr_size << "-" << num_liv_thr << "-" << max_queue_capacity << endl;
				if (queue->curr_size == min(max_queue_capacity, num_liv_thr)){
					sem_post(&full_queue_sem);
				}
			num_liv_thr_mut.unlock();
		sem_post(&queue_mut);
	}
	sem_wait(&Semaphores[num]);
	sem_wait(&queue_mut);
	num_liv_thr_mut.lock();
		num_liv_thr--;
			print.lock();
			//cout << queue->curr_size << "-" << num_liv_thr << "-" << max_queue_capacity << endl;
			print.unlock();
		if (queue->curr_size == num_liv_thr) {
			//cout << "a\n";
			//cout << queue->curr_size << "-" << num_liv_thr << "-" << max_queue_capacity << endl;
			sem_post(&full_queue_sem);
		}
	num_liv_thr_mut.unlock();
	sem_post(&queue_mut);
}

void main_thread(SSTF_Queue *queue, int num)
{
	tuple<int, int> tuple;
	
	while (queue->curr_size > 0 || num_liv_thr > 0)
	{
		//cout << "b\n";
		sem_wait(&full_queue_sem);
		num_liv_thr_mut.lock();
			if (num_liv_thr == 0) {
				break;
			}
		num_liv_thr_mut.unlock();
		//cout << "c\n";
		sem_wait(&queue_mut);
			print.lock();
				tuple = queue->take();
				cout << "service requester " << get<1>(tuple) << " thread " << get<0>(tuple) << endl;
				//cout << "num_liv_thr: " << num_liv_thr << endl;
			print.unlock();
		sem_post(&queue_mut);
		sem_post(&max_queue_capacity_sem);
		sem_post(&Semaphores[get<1>(tuple)]);
		//cout << queue->curr_size << "-" << num_liv_thr << endl;
	}
}

int main(int argc, char const *argv[])
{
	int threads_num = argc-2;
	num_liv_thr = argc-2;
	max_queue_capacity = atoi(argv[1]);
	thread Threads[threads_num+1];
	Semaphores = new sem_t[threads_num];
	SSTF_Queue queue = SSTF_Queue(max_queue_capacity);

	sem_init(&full_queue_sem, 0, 0);
	sem_init(&max_queue_capacity_sem, 0, max_queue_capacity);
	sem_init(&queue_mut, 0, 1);

	for (int i = 0; i < threads_num; i++)
	{
		Threads[i] = thread(&thread_handling, argv[i+2], i, &queue);
		sem_init(&Semaphores[i], 0, 1);
	}
	Threads[threads_num] = thread(&main_thread, &queue, threads_num);
	for (int i = 0; i < argc-1; i++)
	{
		Threads[i].join();
	}
	return 0;
}