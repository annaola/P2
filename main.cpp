#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <tuple>

using namespace std;

struct node
{
	int val, truck;
	struct node *prev;
	struct node *next;
};

class SSTF_Queue
{
	int capacity;
	node *front, *curr;

public:
	vector<int> Trucks;
	int curr_size, trucks_numb;

	//SSTF_Queue() : capacity{0}, front{NULL}, curr{NULL}, curr_size{0} {}

	SSTF_Queue(int capacity, int trucks_num) : capacity{capacity}, front{new node}, curr{new node} , curr_size{0}, trucks_numb{trucks_num}
	{
		front->val = 0;
		front->truck = -1;
		front->prev = NULL;
		front->next = NULL;
		curr = front;
		for (int i = 0; i < trucks_num; i++)
		{
			Trucks.push_back(0);
		}
	}

	bool insert(int val, int truck)
	{
		node *temp, *ptr;

		temp = new node;
		temp->val = val;
		temp->truck = truck;

		if (curr_size < capacity && Trucks[truck] == 0)
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
			Trucks[truck] = 1;
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
		int curr_val, truck;
		node *temp = curr;

		curr_val = curr->val;

		if (curr_size == 0)
		{
			cout << "Kolejka pusta";
		}
		else
		{
			if (curr->prev == NULL)
			{
				curr = curr->next;
				curr->prev = NULL;
				front = curr;
				ret = curr->val;
				truck = curr->truck;
			}
			else if (curr->next == NULL)
			{
				curr = curr->prev;
				curr->next = NULL;
				ret = curr->val;
				truck = curr->truck;
			}
			else if (curr_val - curr->prev->val < curr->next->val - curr_val)
			{
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
				curr = curr->prev;
				ret = curr->val;
				truck = curr->truck;
			}
			else
			{
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
				curr = curr->next;
				ret = curr->val;
				truck = curr->truck;
			}
			free(temp);
			Trucks[truck] = 0;
			curr_size--;
		}
		return make_tuple(ret, truck);
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
	}
};

mutex mut;

void thread_handling(string name, int num, SSTF_Queue *queue)
{
	vector<int> trucks;
	bool insert;
	fstream file;
	file.open(name, ios::in);
	if (file.good())
	{
		int line = 0;
		string truck;
		while (!file.eof())
		{
			getline(file, truck);
			trucks.push_back(atoi(truck.c_str()));
			line++;
		}
	}
	else cout << "Błąd dostępu do pliku" << endl;

	for (size_t i = 0; i < trucks.size(); i++)
	{
		insert = false;
		while (insert == false)
		{
			mut.lock();
			insert = queue->insert(trucks[i], num);
			if (insert == true)
			{
				cout << "requester " << num << " track " << trucks[i] << endl;
			}
			mut.unlock();
		}
	}
	queue->trucks_numb--;
}

void main_thread(SSTF_Queue *queue)
{
	tuple<int, int> tuple;
	while (queue->trucks_numb > 0)
	{
		if (queue->curr_size > 0)
		{
			mut.lock();
			tuple = queue->take();
			cout << "service requester " << get<1>(tuple) << " track " << get<0>(tuple) << endl;
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
	Threads[argc-2] = thread(&main_thread, &queue);
	for (int i = 0; i < argc-1; i++)
	{
		Threads[i].join();
	}
	return 0;
}