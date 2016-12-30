#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <mutex>

using namespace std;

struct node
{
	int val;
	struct node *prev;
	struct node *next;
};

class SSTF_Queue
{
	int capacity;
	node *front, *curr;

public:
	int curr_size;

	//SSTF_Queue() : capacity{0}, front{NULL}, curr{NULL}, curr_size{0} {}

	SSTF_Queue(int capacity) : capacity{capacity}, front{new node}, curr{new node}, curr_size{0}
	{
		front->val = 0;
		front->prev = NULL;
		front->next = NULL;
		curr = front;
	}

	void insert(int val)
	{
		node *temp, *ptr;

		temp = new node;
		temp->val = val;

		if (curr_size < capacity)
		{
			if (val < front->val)
			{
				temp->next = front;
				front->prev = temp;
				front = temp;
			}
			else
			{
				ptr = front;

				while (ptr->next != NULL && ptr->next->val <= val)
				{
					ptr = ptr->next;
				}
				temp->next = ptr->next;
				temp->prev = ptr;
				ptr->next = temp;
			}
			curr_size++;
		}
		else
		{
			cout << "Kolejka pełna" << endl;
		}
	}

	int take()
	{
		int ret = -1;
		int curr_val;
		node *temp = curr;

		curr_val = curr->val;

		if (curr_size == 1)
		{
			cout << "Kolejka pusta";
		}
		else
		{
			if (curr->prev == NULL)
			{
				//cout << "tutaj\n";
				curr = curr->next;
				curr->prev = NULL;
				front = curr;
				ret = curr->val;
			}
			else if (curr->next == NULL)
			{
				curr = curr->prev;
				curr->next = NULL;
				ret = curr->val;
			}
			else if (curr_val - curr->prev->val < curr->next->val - curr_val)
			{
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
				curr = curr->prev;
				ret = curr->val;
			}
			else
			{
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
				curr = curr->next;
				ret = curr->val;
			}
			free(temp);
		}
		return ret;
	}

	void print()
	{
		node *pom;
		pom = front;

		cout << curr->val <<": ";

		if (front == NULL) 
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
			pom = pom->next;
			while (pom != NULL)
			{
				cout << pom->val << " ";
				pom = pom->next;
			}
			cout << endl;
		}
		/*cout << "Aktualna wartość: " << curr->val << endl;
		cout << "Następna wartość: " << curr->next->val << endl;
		if (curr->prev != NULL)
		{
			cout << "Poprzednia wartość: " << curr->prev->val << endl;
		}*/
	}
};

mutex mut;

void thread_handling(string name)
{
	vector<int> trucks;
	fstream file;
	file.open(name, ios::in);
	if (file.good())
	{
		int line = 0;
		string truck;
		while (!file.eof())
		{
			getline(file, truck);
			//cout << truck << " ";
			trucks.push_back(atoi(truck.c_str()));
			line++;
		}
	}
	else cout << "Błąd dostępu do pliku" << endl;
	mut.lock();
	for (size_t i = 0; i < trucks.size(); i++)
	{
		cout << trucks[i] << " ";
	}
	cout << endl;
	mut.unlock();
}


int main(int argc, char const *argv[])
{
	thread Threads[argc-2];
	//SSTF_Queue queue = SSTF_Queue(atoi(argv[1]));
	//SSTF_Queue *q = & queue;

	for (int i = 0; i < argc-2; i++)
	{
		Threads[i] = thread(&thread_handling, argv[i+2]);
	}
	for (int i = 0; i < argc-2; i++)
	{
		Threads[i].join();
	}

	//thread_handling(argv[2], q);

	/*SSTF_Queue queue(10);
	for (int i = 1; i < 11; i++)
	{
		queue.insert(i);
	}
	queue.insert(3);
	queue.insert(1);
	queue.insert(7);
	queue.insert(5);
	queue.print();
	queue.take();
	queue.print();
	queue.take();
	queue.print();
	queue.insert(2);
	queue.print();
	queue.take();
	queue.print();*/
	return 0;
}