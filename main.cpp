#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <tuple>

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
	vector<int> Threads;								// vector, który będzie wskazywać, czy dany wątek ma już element w kolejce
	int capacity, curr_size, threads_numb, active_t;	// pojemność i aktualny rozmiar kolejki, liczba wątków, które nie mają elementów w kolejce oraz liczba aktywnych jeszcze wątków

	SSTF_Queue(int capacity, int threads_num) : first{new node}, last{new node}, curr{new node},
												capacity{capacity}, curr_size{0}, threads_numb{threads_num}, active_t{threads_num}
	{
		first->val = 0;
		first->thread = -1;
		first->prev = NULL;
		first->next = NULL;
		curr = first;
		last = first;
		for (int i = 0; i < threads_num; i++)
		{
			Threads.push_back(0);
		}
	}

	bool insert(int val, int thread)
	{
		node *temp, *ptr1, *ptr2;

		temp = new node;
		temp->val = val;
		temp->thread = thread;

		if (curr_size < capacity && Threads[thread] == 0)
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
			Threads[thread] = 1;
			threads_numb--;
			return true;
		}
		else
		{
			return false;
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
			Threads[thread] = 0;
			curr_size--;
			threads_numb++;
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

mutex mut;

void thread_handling(string name, int num, SSTF_Queue *queue)
{
	vector<int> threads;
	bool insert;
	fstream file;
	file.open(name, ios::in);
	if (file.good())
	{
		int line = 0;
		string thread;
		while (!file.eof())
		{
			getline(file, thread);
			threads.push_back(atoi(thread.c_str()));
			line++;
		}
	}
	else cout << "Błąd dostępu do pliku" << endl;

	//cout << threads.size() << endl;
	for (size_t i = 0; i < threads.size(); i++)
	{
		//cout << i << endl;
		insert = false;
		while (insert == false)
		{
			mut.lock();
			insert = queue->insert(threads[i], num);
			if (insert == true)
			{
				cout << "requester " << num << " thread " << threads[i] << endl;
				//queue->print();
				queue->threads_numb--;
				if (i == threads.size()-1)
				{
					queue->active_t--;
				}
			}
			mut.unlock();
		}
	}
}

void main_thread(SSTF_Queue *queue, int num)
{
	tuple<int, int> tuple;
	while (queue->curr_size < queue->capacity) {}
	while (queue->curr_size > 0)
	{
		//cout << queue->threads_numb << "," << queue->active_t << endl;
		if (queue->curr_size == queue->capacity || queue->threads_numb == queue->active_t || queue->active_t == 0)
		{
			mut.lock();
			tuple = queue->take();
			cout << "service requester " << get<1>(tuple) << " thread " << get<0>(tuple) << endl;
			//queue->print();
			queue->threads_numb++;
			mut.unlock();			
		}
	}
}


int main(int argc, char const *argv[])
{
	thread Threads[argc-1];
	SSTF_Queue queue = SSTF_Queue(atoi(argv[1]), argc-2);

	for (int i = 0; i < argc-2; i++)
	{
		Threads[i] = thread(&thread_handling, argv[i+2], i, &queue);
	}
	Threads[argc-2] = thread(&main_thread, &queue, argc-2);
	for (int i = 0; i < argc-1; i++)
	{
		Threads[i].join();
	}
	return 0;
}