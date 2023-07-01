#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

using namespace std;

class FineGrainedQueue {
public:
    // Инициализировать head_ так, чтобы он указывал на tail_ (пустой список).
    FineGrainedQueue() {
        tail_ = new Node(0);
        head_ = new Node(0, tail_);
    }

    /*
    * Вставьте узел со значением val в позицию pos.
    *
    * Для обеспечения взаимного исключения блокировки текущего узла и
    * предыдущего узла, узлы  должны быть получены  для продолжения  работы вставки. Как только
    * блокировка получены, код делает примерно следующее:
    *
    *      | prev -> node | prev -> node | prev    node |
    *      |              |          ^   |   v      ^   |
    *      |   new_node   |   new_node   |   new_node   |
    */
    void insert(int value, int pos) {
        Node* new_node = new Node(value);
        Node* prev = head_;
        unique_lock<mutex> prev_lk(prev->m);
        Node* node = prev->next;
        unique_lock<mutex> node_lk(node->m);
        for (int i = 0; i < pos && node != tail_; i++) {
            prev = node;
            node = node->next;
            prev_lk.swap(node_lk);
            node_lk = unique_lock<mutex>(node->m);
        }
        new_node->next = node;
        prev->next = new_node;
    }

    /*
    * Удалить  узел в позиции поз.
    *
    * Чтобы обеспечить  исключение взаимной блокировки, выполните шаги из insert(). Как только
    *  lock получены,
    * код делает примерно следующее:
    *
    *    (*)     (*)            (*)     (*)            (*)
    *  | prev -> node -> next | prev    node -> next | prev ---------> next |
    *  |                      |   v--------------^   |                      |
    *
    * (*) выделяет узлы, блокировки которых наступила.
    */
    void erase(int pos) {
        Node* prev = head_;
        unique_lock<mutex> prev_lk(prev->m);
        Node* node = prev->next;
        unique_lock<mutex> node_lk(node->m);
        for (int i = 0; i < pos && node != tail_; i++) {
            prev = node;
            node = node->next;
            prev_lk.swap(node_lk);
            node_lk = unique_lock<mutex>(node->m);
        }
        if (node == tail_) {
            return;
        }
        prev->next = node->next;
        node_lk.unlock();
        delete node;
    }

    int get(int pos) {
        Node* prev = head_;
        unique_lock<mutex> prev_lk(prev->m);
        Node* node = prev->next;
        unique_lock<mutex> node_lk(node->m);
        for (int i = 0; i < pos && node != tail_; i++) {
            prev = node;
            node = node->next;
            prev_lk.swap(node_lk);
            node_lk = unique_lock<mutex>(node->m);
        }
        if (node == tail_) {
            return 0;
        }
        return node->value;
    }

private:
    struct Node {
        int value;
        Node* next;
        mutex m;

        Node(int val_, Node* next_ = nullptr) : value(val_), next(next_) {}
    };

    Node* head_, * tail_;
};

void testNThread(int n) {
    const int N = 10;
    vector<thread> threads(n);
    FineGrainedQueue lst;
    for (int i = 0; i < n; i++) {
        threads[i] = thread([i, &lst, N]() {
            for (int j = 0; j < N; j++) {
                lst.insert(i, 0);
            }
            });
    }
    for (int i = 0; i < n; i++) {
        threads[i].join();
    }
    for (int i = 0; i < N * n; i++) {
        int val = lst.get(0);
        lst.erase(0);
        cout << val << " ";
    }
    cout << "\n";
}

int main(int argc, char** argv) {
    int n = 10;
    if (argc >= 2) {
        n = atoi(argv[1]);
    }
    testNThread(n);
}