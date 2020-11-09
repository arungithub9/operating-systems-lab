//LAB4

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <stdio.h>
#include <getopt.h>
#include <queue>
#include <list>
#include <math.h>
using namespace std;

int system_time = 1;
int total_time = 0;
int tot_movement = 0;
double avg_turnaround = 0;
double avg_waittime = 0;
int max_waittime = 0;
int callno = 1;
char choice;

int num_io = 0;
int track_pos = 0;
bool track_dir_low_hi = true;

struct ioIns
{
    int id;
    int arrival_time;
    int start_time;
    int end_time;
    int track_no;

    bool operator==(const ioIns &s) const { return id == s.id; }
    bool operator!=(const ioIns &s) const { return !operator==(s); }
};
queue<ioIns> ins_que;
list<ioIns> pool;
queue<ioIns> done_que;
vector<ioIns> fstats;
ioIns *active = NULL;

list<ioIns> Que1;
list<ioIns> Que2;

list<ioIns> *activeQue = &Que1;
list<ioIns> *addQue = &Que2;

bool orig = true;

class IOSchedulerBase
{
public:
    virtual void strategy_get_next() = 0;
};
void track_dir_change()
{
    if (active->track_no > track_pos && !track_dir_low_hi)
        track_dir_low_hi = true;
    if (active->track_no < track_pos && track_dir_low_hi)
        track_dir_low_hi = false;
}
class FIFO : public IOSchedulerBase
{
public:
    void strategy_get_next()
    {
        active = &pool.front();
        track_dir_change();
    }
};


class LOOK4 : public IOSchedulerBase
{
public:
    void strategy_get_next()
    {
        if (track_dir_low_hi)
        {
            //if only one element, use it
            //else look for element to right
            //else look for left element

            if (pool.size() == 1)
            {
                active = &pool.front();
                if (active->track_no < track_pos)
                {
                    track_dir_low_hi = false;
                }
            }

            else
            {
                //look for right or on same position

                list<ioIns>::iterator ijt;
                int min = 10000;
                for (ijt = pool.begin(); ijt != pool.end(); ++ijt)
                {
                    if (ijt->track_no >= track_pos)
                    {
                        int dist = ijt->track_no - track_pos;
                        if (dist < min)
                        {
                            min = dist;
                            active = &*ijt;
                        }
                    }
                }

                //if right or same nf, check left

                if (active == NULL)
                {   
                    min = 10000;
                    for (ijt = pool.begin(); ijt != pool.end(); ++ijt)
                    {
                        if (ijt->track_no < track_pos)
                        {
                            int dist = track_pos - ijt->track_no;
                            if (dist < min)
                            {
                                min = dist;
                                active = &*ijt;
                            }
                        }
                    }

                    track_dir_low_hi = false;
                }

            }
        }
        else
        {
            //track dir hi to lo
            if (pool.size() == 1)
            {
                active = &pool.front();
                if (active->track_no > track_pos)
                {
                    track_dir_low_hi = true;
                }
            }

            else
            {
                list<ioIns>::iterator ijt;
                int min = 10000;
                for (ijt = pool.begin(); ijt != pool.end(); ++ijt)
                {
                    if (track_pos >= ijt->track_no)
                    {
                        int dist = track_pos-ijt->track_no;
                        if (dist < min)
                        {
                            min = dist;
                            active = &*ijt;
                        }
                    }
                }

                //check right

                if (active == NULL)
                {   
                    min = 10000;
                    for (ijt = pool.begin(); ijt != pool.end(); ++ijt)
                    {
                        if (ijt->track_no > track_pos)
                        {
                            int dist = ijt->track_no - track_pos;
                            if (dist < min)
                            {
                                min = dist;
                                active = &*ijt;
                            }
                        }
                    }

                    track_dir_low_hi = true;
                }
            }
            

        }
        
    }
};

class FLOOK : public IOSchedulerBase
{
public:
    void strategy_get_next()
    {
        if (track_dir_low_hi)
        {
            //if only one element, use it
            //else look for element to right
            //else look for left element

            if (activeQue->size() == 1)
            {
                active = &activeQue->front();
                if (active->track_no < track_pos)
                {
                    track_dir_low_hi = false;
                }
            }

            else
            {
                //look for right or on same position

                list<ioIns>::iterator ijt;
                int min = 10000;
                for (ijt = activeQue->begin(); ijt != activeQue->end(); ++ijt)
                {
                    if (ijt->track_no >= track_pos)
                    {
                        int dist = ijt->track_no - track_pos;
                        if (dist < min)
                        {
                            min = dist;
                            active = &*ijt;
                        }
                    }
                }

                //if right or same nf, check left

                if (active == NULL)
                {   
                    min = 10000;
                    for (ijt = activeQue->begin(); ijt != activeQue->end(); ++ijt)
                    {
                        if (ijt->track_no < track_pos)
                        {
                            int dist = track_pos - ijt->track_no;
                            if (dist < min)
                            {
                                min = dist;
                                active = &*ijt;
                            }
                        }
                    }

                    track_dir_low_hi = false;
                }

            }
        }
        else
        {
            //track dir hi to lo
            if (activeQue->size() == 1)
            {
                active = &activeQue->front();
                if (active->track_no > track_pos)
                {
                    track_dir_low_hi = true;
                }
            }

            else
            {
                list<ioIns>::iterator ijt;
                int min = 10000;
                for (ijt = activeQue->begin(); ijt != activeQue->end(); ++ijt)
                {
                    if (track_pos >= ijt->track_no)
                    {
                        int dist = track_pos-ijt->track_no;
                        if (dist < min)
                        {
                            min = dist;
                            active = &*ijt;
                        }
                    }
                }

                //check right

                if (active == NULL)
                {   
                    min = 10000;
                    for (ijt = activeQue->begin(); ijt != activeQue->end(); ++ijt)
                    {
                        if (ijt->track_no > track_pos)
                        {
                            int dist = ijt->track_no - track_pos;
                            if (dist < min)
                            {
                                min = dist;
                                active = &*ijt;
                            }
                        }
                    }

                    track_dir_low_hi = true;
                }
            }
            

        }
        
    }

};

class SSTF : public IOSchedulerBase
{
public:
    void strategy_get_next()
    {
        if (pool.size() == 1)
        {
            active = &pool.front();
        }
        else
        {
            list<ioIns>::iterator ijt;
            int dist = 10000;
            for (ijt = pool.begin(); ijt != pool.end(); ++ijt)
            {
                int path = abs(ijt->track_no - track_pos);
                if (path < dist)
                {
                    dist = path;
                    active = &*ijt;
                }
            }
        }

        track_dir_change();
    }
};

class CLOOK3 : public IOSchedulerBase
{
public:
    void strategy_get_next()
    {
        ioIns *leftFarthest;
        ioIns *rightNearest;

        bool lset = false;
        bool rset = false;

        list<ioIns>::iterator t = pool.begin();

        int lfar = 10000;
        int rnear = 10000;

        for (t = pool.begin(); t != pool.end(); ++t)
        {
            int target = t->track_no;
            if (target >= track_pos)
            {
                if (target < rnear)
                {
                    rnear = target;
                    rightNearest = &*t;
                    rset = true;
                }
            }
            else
            {
                if (target < lfar)
                {
                    lfar = target;
                    leftFarthest = &*t;
                    lset = true;
                }
            }
        }

        if (track_dir_low_hi == true)
        {
            if (rset)
            {
                active = rightNearest;
            }
            else
            {
                active = leftFarthest;
                track_dir_low_hi = false;
            }
        }
        else
        {
            if (rset)
            {
                active = rightNearest;
                track_dir_low_hi = true;
            }
            else
            {
                active = leftFarthest;
            }
        }
    }
};

void readFile(string input_file)
{
    ifstream in(input_file);
    if (!in)
    {
        cout << "error opening file";
        exit(0);
    }

    string s;
    getline(in, s);
    while (s[0] == '#')
    {
        getline(in, s);
    }
    char *x;
    int ins_no = -1;
    int arr_time;
    int track_num;
    while (in)
    {
        arr_time = atoi(strtok(&s[0], " "));
        track_num = atoi(strtok(NULL, " "));
        getline(in, s);
        ins_no++;

        ioIns inst;
        inst.id = ins_no;
        inst.arrival_time = arr_time;
        inst.track_no = track_num;
        ins_que.push(inst);
        fstats.push_back(inst);

        if (!in)
            break;
    }
}

void checker()
{
    list<ioIns> x;
    ioIns inh;
    for (int i = 0; i < 5; i++)
    {
        inh.id = i;
        x.push_back(inh);
    }

    for (int i = 0; i < 5; i++)
    {
        inh.id = i;
        x.remove(inh);
    }
}

void completeIO()
{
    active->end_time = system_time;
    int id = active->id;
    fstats[id] = *active;
    done_que.push(*active);
    ioIns insxn = *active;
    if (choice == 'f')
        activeQue->remove(insxn);
    else
        pool.remove(insxn);
    active = NULL;
}

int main(int argc, char **argv)
{
    int c = getopt(argc, argv, "s:");
    string inp_file = argv[optind];
    readFile(inp_file);

    num_io = ins_que.size();

    choice = optarg[0];

    IOSchedulerBase *b;

    switch (choice)
    {
    case ('i'):
        b = new FIFO();
        break;

    case ('j'):
        b = new SSTF();
        break;

    case ('s'):
        b = new LOOK4();
        break;

    case ('c'):
        b = new CLOOK3();
        break;

    case ('f'):
        b = new FLOOK();

    default:
        break;
    }

    ioIns inst;
    int ct = 0;

    while (true && choice != 'f')
    {
        if (done_que.size() == num_io)
            break;

        //s1 : checking if new io has entered at this point of time
        if (ins_que.size() != 0)
        {
            int t = ins_que.front().arrival_time;
            if (system_time == t)
            {
                inst = ins_que.front();
                ins_que.pop();
                pool.push_back(inst);
            }
        }

        //s2 : if there is an active io uncompleted, check if completed
        bool io_complete = false;

        if (active != NULL)
        {
            //checking completion
            if (track_pos == active->track_no)
            {
                //io completion formalities
                completeIO();
                io_complete = true;
                if (done_que.size() == num_io)
                    break;
            }
        }

        // s3 : if not completed move track

        if (active != NULL)
        {
            if (track_dir_low_hi)
            {
                track_pos++;
                tot_movement++;
            }
            else
            {
                track_pos--;
                tot_movement++;
            }
        }

        // s4 if no active io fetch one

        if (active == NULL && pool.size() > 0)
        {
            b->strategy_get_next();
            active->start_time = system_time;
            if (active->track_no == track_pos)
            {
                completeIO();
                active = NULL;
                continue;
            }
            else
            {
                if (track_dir_low_hi)
                {
                    track_pos++;
                    tot_movement++;
                }
                else
                {
                    track_pos--;
                    tot_movement++;
                }
            }
        }
        //s5
        system_time++;
    }

    while (true && choice == 'f')
    {
        //s1
        if (ins_que.size() != 0)
        {
            int t = ins_que.front().arrival_time;
            if (system_time == t)
            {
                inst = ins_que.front();
                ins_que.pop();
                addQue->push_back(inst);
            }
        }

        bool io_complete = false;

        //s2
        if (active != NULL)
        {
            //checking completion
            if (track_pos == active->track_no)
            {
                //io completion formalities
                completeIO();
                io_complete = true;
                active = NULL;
                if (done_que.size() == num_io)
                    break;
            }
        }

        //s3
        if (active != NULL)
        {
            if (track_dir_low_hi)
            {
                track_pos++;
                tot_movement++;
            }
            else
            {
                track_pos--;
                tot_movement++;
            }
        }

        //s4
        if (active == NULL && activeQue->size() > 0)
        {
            b->strategy_get_next();
            active->start_time = system_time;
            if (active->track_no == track_pos)
            {
                completeIO();
                active = NULL;
                if (done_que.size() == num_io)
                {
                    break;
                }
                continue;
            }
            else
            {
                if (track_dir_low_hi)
                {
                    track_pos++;
                    tot_movement++;
                }
                else
                {
                    track_pos--;
                    tot_movement++;
                }
            }
        }
        else if (active == NULL)
        {
            if (addQue->size() == 0)
            {
                system_time++;
                continue;
            }
            else
            {
                if (orig)
                {
                    activeQue = &Que2;
                    addQue = &Que1;
                    orig = false;
                }
                else
                {
                    activeQue = &Que1;
                    addQue = &Que2;
                    orig = true;
                }
                continue;
            }
        }
        else
            ;

        system_time++;
    }

    total_time = system_time;

    int waittime = 0;
    for (int k = 0; k < fstats.size(); k++)
    {
        printf("%5d: %5d %5d %5d\n", k, fstats[k].arrival_time, fstats[k].start_time, fstats[k].end_time);
        avg_turnaround = avg_turnaround + fstats[k].end_time - fstats[k].arrival_time;
        waittime = fstats[k].start_time - fstats[k].arrival_time;
        avg_waittime = avg_waittime + waittime;
        if (waittime > max_waittime)
            max_waittime = waittime;
    }
    int sz = fstats.size();
    avg_turnaround = avg_turnaround / sz;
    avg_waittime = avg_waittime / sz;

    printf("SUM: %d %d %.2lf %.2lf %d\n", total_time, tot_movement, avg_turnaround, avg_waittime, max_waittime);

    checker();
}
