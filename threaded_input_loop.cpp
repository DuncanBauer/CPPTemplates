#include <iostream>
#include <ios>
#include <limits>
#include <mutex>
#include <thread>
#include <queue>

std::mutex m;
std::priority_queue<int> q;

void threadFunc(int);
void pushToStack(int);

void threadFunc(int n = 10)
{
    for(int i = 0; i < n; ++i)
    {
        pushToStack(i);
    }
}

void pushToStack(int i)
{
    std::lock_guard<std::mutex> lock(m);
    q.push(i);
}

void inputLoop()
{
    bool q = false;
    char cmd;
    
    while(!q)
    {
        std::cout << "Quit: (Q or q)" << '\n';
        std::cout << "Enter command: " << '\n' << "> ";
        std::cin >> cmd;
        switch(cmd)
        {
            case 'Q':
                [[fallthrough]];
            case 'q':
                q = true;
                break;
            default:
                break;
        }
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int main(int argc, char* argv[])
{
    std::thread t1(inputLoop);
    std::thread t2(threadFunc, 100);
    std::thread t3([]{ threadFunc(); }); // Thread function with a default parameter must be called via lambda
    
    // Do stuff here
    
	t1.join();
	t2.join();
	t3.join();
	
    std::cout << '\n';
    std::cout << "Queue length: " << q.size() << '\n';
    int index = 0;
    while(q.size())
    {
        std::cout << index << ": " << q.top() << '\n';
        q.pop();
        index++;
    }
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	
    return 0;
}
