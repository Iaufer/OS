#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
using namespace std;

int N = 5;

struct Monitor{
    mutex mutex;
    condition_variable c_var;
    int data;
    bool data_ready;
};

void provide(Monitor& monitor){
    //srand(time(NULL));

    while(N != 0){
        int data = rand();
        unique_lock<mutex> lock(monitor.mutex);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        while(monitor.data_ready){
            monitor.c_var.wait(lock);
            continue;
        }

        monitor.data = data;
        monitor.data_ready = true;

        monitor.c_var.notify_all();

        N--;
    }

    return;
}

void consume(Monitor& monitor){
    unique_lock<mutex> lock(monitor.mutex);
    
    while(!monitor.data_ready){
        monitor.c_var.wait(lock);
    }

    int data = monitor.data;
    monitor.data_ready = false;

    monitor.c_var.notify_all();

    cout << "DATA = " << data << endl;
}


int main(int argc, char const *argv[])
{
    srand(time(NULL));
    
    vector<thread*> th;
    Monitor monitor;
    monitor.data = 0;
    monitor.data_ready = false;

    // thread _provide(provide, ref(monitor));
    // thread c1(consume, ref(monitor));
    // thread c2(consume, ref(monitor));
    // thread c3(consume, ref(monitor));
    // thread c4(consume, ref(monitor));
    // thread c5(consume, ref(monitor));
    // _provide.join();
    // c1.join();
    // c2.join();
    // c3.join();
    // c4.join();
    // c5.join();


    for(int i = 0; i < N; i++){
        th.push_back(new thread (consume, ref(monitor)));
    }

    thread _provide(provide, ref(monitor));

    for(auto threadPtr : th){
        threadPtr -> join();
        delete threadPtr;
    }    

    _provide.join();

    return 0;
}
