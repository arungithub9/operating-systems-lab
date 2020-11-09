#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <fstream>
#include <stdio.h>
#include <getopt.h>
#include <bits/stdc++.h>

using namespace std;

//bool options
bool O_option = false;
bool P_option = false; //pagetable option
bool F_option = false; //frame table option
bool S_option = false; //summary line
bool x_option = false; //prints the current page table after each instructions (see example outputs) and this should help you significantly to track down bugs and transitions (remember you write the print function only once
bool y_option = false; //like ‘x’ but prints the page table of all processes instead.
bool f_option = false; //prints the frame table after each instruction.
bool a_option = false; //prints additional “aging” information during victim_selecton and after each instruction for complex algorithms

//other globals
const int pte_num = 64; //page table entries
int num_frames = 128;   //num frames, max = 128
int algChoice;          //choice of scheduler
int rand_nums;          //total random nums in file
int ins_counter = 0;    //instruction counter

int current_proc_number = 0;

unsigned long long total_cost = 0;
unsigned long long total_cost2 = 0;
unsigned long long total_inst_count = 0;
unsigned long long total_context_switches = 0;
unsigned long long total_proc_exits = 0;
unsigned long long rwaccesses = 0;

deque<int> randoms;
int rofs = 0;

int get_next_random()
{
    if (rofs == rand_nums)
    {
        rofs = 0;
    }
    //cout<<randoms.size()<<rand_nums<<endl;
    int ans = randoms[rofs] % num_frames;
    ++rofs;
    return ans;
}

struct inst
{
    char instr;
    int addr;
};
vector<inst> instructions;

struct PTE
{
    unsigned int present : 1; // present/valid bit
    unsigned int write_protect : 1;
    unsigned int modified : 1;
    unsigned int referenced : 1;
    unsigned int pagedout : 1;
    unsigned int frame_number : 7;
    unsigned int filemap : 1;
    unsigned int hole : 1;

    void setPTE()
    {
        present = 0;
        write_protect = 0;
        modified = 0;
        referenced = 0;
        pagedout = 0;
        frame_number = 0;
        filemap = 0;
        hole = 0;
    }
};

struct FTE
{
    int frameNum;
    int procID;
    int vpage;
    unsigned long age;
    bool victimed;
    unsigned long tau;
};
deque<FTE> frameTable;
deque<FTE *> freeFrames;

int usedFramesCt = 0;

struct VMA
{
    unsigned int start_vpage : 7;
    unsigned int end_vpage : 7;
    unsigned int write_protected : 1;
    unsigned int file_mapped : 1;
};

class Process
{
public:
    vector<VMA> virtualMemoryAreasList;
    vector<PTE> pageTable;
    int pid;
    unsigned long long unmaps;
    unsigned long long maps;
    unsigned long long pageins;
    unsigned long long pageouts;
    unsigned long long fins;
    unsigned long long fouts;
    unsigned long long zeros;
    unsigned long long segv;
    unsigned long long segprot;

    void setProcess()
    {
        for (int i = 0; i < pte_num; i++)
        {
            PTE e;      //empty page table entry
            e.setPTE(); //initialize with empty vals
            e.frame_number = i;
            pageTable.push_back(e);
        }

        pid = unmaps = maps = pageins = pageouts = fins = fouts = zeros = segv = segprot = 0;
    }
};
vector<Process> processes;
Process *current_process;
class BasePager
{
public:
    virtual FTE *getVictimFrame() = 0;
};

class FIFO : public BasePager //done
{
public:
    FTE *pointer;
    int size;
    int ct;

    FIFO()
    {
        ct = 0;
        size = frameTable.size();
        pointer = &frameTable.front();
    }
    FTE *getVictimFrame()
    {

        FTE *victim = pointer;
        ct++;
        if (ct == size)
        {
            ct = 0;
            pointer = &frameTable.front();
        }
        pointer = &frameTable[ct];
        victim->victimed = true;
        return victim;
    }
};

class Random : public BasePager //done
{
public:
    Random()
    {
        //nothing
    }
    FTE *getVictimFrame()
    {
        int index = get_next_random();
        frameTable[index].victimed = true;
        return &frameTable[index];
    }
};

class Clock : public BasePager
{
public:
    FTE *pointer;
    FTE *ans;
    int size;
    int ct;

    Clock()
    {
        ct = 0;
        size = frameTable.size();
        pointer = &frameTable[0];
    }
    FTE *getVictimFrame()
    {
        PTE *correspondingPage = &processes[pointer->procID].pageTable[pointer->vpage];
        while (correspondingPage->referenced == 1)
        {
            correspondingPage->referenced = 0;
            ct++;
            if (ct == size)
                ct = 0;
            pointer = &frameTable[ct];
            correspondingPage = &processes[pointer->procID].pageTable[pointer->vpage];
        }

        if (correspondingPage->referenced == 0)
        {
            ans = &frameTable[correspondingPage->frame_number];
            ans->victimed = true;
            ct++;
            if (ct == size)
                ct = 0;
            pointer = &frameTable[ct];
        }

        return ans;
    }
};

class NRU : public BasePager
{
public:
    int handpos;
    int nruClass[4];
    FTE *victim;
    int last;
    NRU()
    {
        handpos = 0;
        last = 0;
        victim = NULL;
        for (int i = 0; i < 4; i++)
        {
            nruClass[i] = -1;
        }
    }
    FTE *getVictimFrame()
    {
        //fresh reset
        for (int i = 0; i < 4; i++)
        {
            nruClass[i] = -1;
        }

        bool victimSet = false;

        //victim calc
        for (int y = 0; y < frameTable.size(); y++)
        {
            int id = (y + handpos) % frameTable.size();
            FTE *current_frame = &frameTable[id];
            int pid = current_frame->procID;
            int vpageNum = current_frame->vpage;
            PTE *current_page = &processes[pid].pageTable[vpageNum];
            int classId = current_page->referenced * 2 + current_page->modified;

            if (nruClass[classId] == -1)
            {
                if (classId == 0)
                {
                    victim = current_frame;
                    victim->victimed = true;
                    victimSet = true;
                    handpos = (id + 1) % frameTable.size();
                    break;
                }
                else
                {
                    nruClass[classId] = current_frame->frameNum;
                }
            }
        }

        if (!victimSet)
        {
            for (int i = 0; i < 4; i++)
            {
                if (nruClass[i] != -1)
                {
                    victim = &frameTable[nruClass[i]];
                    victim->victimed = true;
                    victimSet = true;
                    handpos = (victim->frameNum + 1) % frameTable.size();
                    break;
                }
            }
        }

        if (ins_counter - last >= 50)
        {
            for (int i = 0; i < frameTable.size(); i++)
            {
                FTE *f = &frameTable[i];
                if (f->procID != -1)
                    processes[f->procID].pageTable[f->vpage].referenced = 0;
            }
            last = ins_counter;
        }

        return victim;
    }
};

class Aging : public BasePager
{
public:
    int handpos;
    FTE *victim;
    Aging()
    {
        handpos = 0; //initial
    }
    FTE *getVictimFrame()
    {
        victim = &frameTable[handpos];

        //s1 lowest age
        for (int i = 0; i < frameTable.size(); i++)
        {
            int id = (i + handpos) % frameTable.size();
            FTE *f = &frameTable[id];
            f->age = f->age >> 1;
            if (processes[f->procID].pageTable[f->vpage].referenced == 1)
            {
                f->age = (f->age | 0x80000000);
                processes[f->procID].pageTable[f->vpage].referenced = 0;
            }

            if (f->age < victim->age) //maintain ord
            {
                victim = f;
            }
        }

        handpos = (victim->frameNum + 1) % frameTable.size();
        victim->victimed = true;
        return victim;
    }
};

class Working_Set : public BasePager
{
public:
    FTE *currentFTE;
    PTE *currentPTE;
    FTE *ans;
    int hand;
    int size;
    int threshold;
    int min;

    Working_Set()
    {
        hand = 0;
        size = frameTable.size();
        threshold = 50;
        ans = NULL;
        min = -1000;
    }
    FTE *getVictimFrame()
    {   
        min = -1000;
        ans = NULL;
        //ans = &frameTable[hand];
        for (int i = 0; i < frameTable.size(); i++)
        {
            int curFrameID = (i + hand) % frameTable.size();
            currentFTE = &frameTable[curFrameID];
            currentPTE = &processes[currentFTE->procID].pageTable[currentFTE->vpage];
            int age = ins_counter - 1 - currentFTE->tau;
            if (currentPTE->referenced)
            {
                currentPTE->referenced = 0;
                currentFTE->tau = ins_counter - 1; //wout working with chk
            }
            else
            {
                if (age >= 50)
                {
                    ans = currentFTE;
                    ans->victimed = true;
                    //ans->tau = ins_counter;
                    hand = (ans->frameNum + 1) % frameTable.size();
                    return ans;
                }
                else
                {
                    if (age > min)
                    {
                        min = age;
                        ans = currentFTE;
                    }
                }
            }
        }
        if(ans == NULL)
        {
            ans = &frameTable[hand];
        }
        hand = (ans->frameNum + 1) % frameTable.size();
        ans->victimed = true;
        //ans->tau = ins_counter;
        return ans;
    }
};

BasePager *pager;

int reader(string input_file, string random_file)
{
    string s;
    int num_proc;
    int num_vmas;
    int start_page;
    int end_page;
    int write_protected;
    int file_mapped;
    ifstream in(input_file);
    if (!in)
    {
        cout << "Cannot open input file.\n";
        exit(0);
    }
    else
    {
        getline(in, s);
        while (s[0] == '#')
            getline(in, s);
        num_proc = atoi(&s[0]);
        getline(in, s);
        while (s[0] == '#')
            getline(in, s);

        for (int i = 0; i < num_proc; i++) //procRead
        {
            Process p;
            p.setProcess();
            p.pid = i;
            num_vmas = atoi(&s[0]);
            getline(in, s);
            for (int j = 0; j < num_vmas; j++)
            {
                char *pch;
                pch = strtok(&s[0], " ");
                start_page = atoi(pch);
                pch = strtok(NULL, " ");
                end_page = atoi(pch);
                pch = strtok(NULL, " ");
                write_protected = atoi(pch);
                pch = strtok(NULL, " ");
                file_mapped = atoi(pch);
                VMA v;
                v.start_vpage = start_page;
                v.end_vpage = end_page;
                v.write_protected = write_protected;
                v.file_mapped = file_mapped;
                p.virtualMemoryAreasList.push_back(v);
                getline(in, s);
                while (s[0] == '#')
                    getline(in, s);
            }

            // for (int y = 0; y < p.virtualMemoryAreasList.size(); y++)
            // {
            //     cout << "sdfsd";
            //     VMA temp = p.virtualMemoryAreasList[y];
            //     for (int x = temp.start_vpage; x <= temp.end_vpage; x++)
            //     {
            //         p.pageTable[x].hole = 0;
            //     }
            // }

            processes.push_back(p);
        }

        if (s[0] == '#')
            getline(in, s);
        else
            while (s[0] != '#' && in)
            {
                inst ins;
                char *pch;
                pch = strtok(&s[0], " ");
                ins.instr = *pch;
                pch = strtok(NULL, " ");
                ins.addr = atoi(pch);
                instructions.push_back(ins);
                getline(in, s);
            }
    }
    in.close();
    ifstream in2(random_file);
    if (!in2)
    {
        cout << "cannot open rand file";
        exit(0);
    }
    else
    {
        getline(in2, s);
        rand_nums = atoi(&s[0]);
        while (getline(in2, s))
        {
            randoms.push_back(atoi(&s[0]));
        }
    }
}

void frameTableinit()
{
    for (int i = 0; i < num_frames; i++) //instantiated the frame table
    {
        FTE f;
        f.frameNum = i;
        f.procID = -1;
        f.vpage = -1;
        f.victimed = false;
        f.age = 0;
        f.tau = 0;
        frameTable.push_back(f);
    }

    for (int i = 0; i < num_frames; i++) //instantiated free frame pointers to the frame table
    {
        FTE *f;
        f = &frameTable[i];
        freeFrames.push_back(f);
    }
}

void checker()
{
    for (int i = 0; i < processes.size(); i++)
    {
        Process x = processes[i];
        cout << x.pid << " " << endl;

        for (int j = 0; j < x.virtualMemoryAreasList.size(); j++)
        {
            cout << x.virtualMemoryAreasList[j].start_vpage << " " << x.virtualMemoryAreasList[j].end_vpage << " " << x.virtualMemoryAreasList[j].write_protected << " " << x.virtualMemoryAreasList[j].file_mapped << endl;
        }

        for (int k = 0; k < x.pageTable.size(); k++)
        {
            cout << x.pageTable[k].frame_number << " : " << x.pageTable[k].hole << endl;
        }

        cout << endl;
        cout << endl;
    }
}

void printPageTable(int pid)
{
    Process x = processes[pid];
    cout << "PT[" << pid << "]: ";
    for (int i = 0; i < pte_num; i++)
    {
        if (x.pageTable[i].present == 1)
        {
            cout << i << ":";

            if (x.pageTable[i].referenced == 1)
                cout << "R";
            else
                cout << "-";
            if (x.pageTable[i].modified == 1)
                cout << "M";
            else
                cout << "-";
            if (x.pageTable[i].pagedout == 1)
                cout << "S ";
            else
                cout << "- ";
        }
        else
        {
            if (x.pageTable[i].pagedout == 1)
                cout << "# ";
            else
                cout << "* ";
        }
    }
    cout << endl;
}

void printFrameTable()
{
    cout << "FT: ";
    for (int i = 0; i < frameTable.size(); i++)
    {
        if (frameTable[i].procID != -1)
        {
            cout << frameTable[i].procID << ":" << frameTable[i].vpage << " ";
        }
        else
        {
            cout << "* ";
        }
    }
    cout << endl;
}

FTE *get_frame()
{
    FTE *f;
    if (freeFrames.size() != 0)
    {
        f = freeFrames.front();
        freeFrames.pop_front();
        usedFramesCt++;
    }
    else
    {
        //need to get victim frame

        f = pager->getVictimFrame();
    }

    return f;
}

void simulator()
{
    switch (algChoice) //creating correct pager class
    {
    case 0:
        pager = new FIFO();
        //cout << " \n created";
        break;
    case 1:
        pager = new Random();
        break;
    case 2:
        pager = new Clock();
        break;
    case 3:
        pager = new NRU();
        break;
    case 4:
        pager = new Aging();
        break;
    case 5:
        pager = new Working_Set();
        break;
    default:
        break;
    }

    //simulation
    for (int i = 0; i < instructions.size(); i++)
    {

        inst currentIns = instructions[i];
        ins_counter++;
        if (O_option)
        {
            cout << i << ": ==> " << currentIns.instr << " " << currentIns.addr << endl;
        }
        //handle e an c. e left todo

        if (currentIns.instr == 'c')
        {
            current_proc_number = currentIns.addr;
            current_process = &processes[current_proc_number];
            //cout<<current_process->pid;
            total_context_switches++;
            total_cost2 += 121;
            continue;
        }

        if (currentIns.instr == 'e')
        {
            PTE *x;
            cout << "EXIT current process " << currentIns.addr << endl;
            for (int i = 0; i < processes[currentIns.addr].pageTable.size(); i++)
            {
                x = &processes[currentIns.addr].pageTable[i];
                if (x->present)
                {
                    FTE *f = &frameTable[x->frame_number];
                    cout << " UNMAP " << f->procID << ":" << f->vpage << endl;
                    f->age = 0;
                    f->procID = -1;
                    f->victimed = false;
                    f->vpage = -1;
                    f->tau = 0;
                    freeFrames.push_back(f);
                    total_cost2 += 400;
                    processes[currentIns.addr].unmaps++;

                    if (x->modified)
                    {
                        if (x->filemap)
                        {
                            total_cost2 += 2500;
                            processes[currentIns.addr].fouts++;
                            cout << " FOUT" << endl;
                        }
                    }
                }
                x->present = 0;
                x->referenced = 0;
                x->pagedout = 0;
            }

            total_proc_exits++;
            total_cost2 += 175;

            continue;
        }
        //handle r and w

        int cur_vpage = currentIns.addr;
        PTE *pte = &current_process->pageTable[cur_vpage];

        total_cost2++;

        if (!pte->present) //if faulting
        {
            bool validAddr = false;
            if (pte->hole == 1) //reduce comp cost
            {
                validAddr = true;
            }
            else
            {
                for (int k = 0; k < current_process->virtualMemoryAreasList.size(); k++)
                {
                    if (cur_vpage >= current_process->virtualMemoryAreasList[k].start_vpage && cur_vpage <= current_process->virtualMemoryAreasList[k].end_vpage)
                    {
                        validAddr = true;
                        pte->filemap = current_process->virtualMemoryAreasList[k].file_mapped;
                        pte->write_protect = current_process->virtualMemoryAreasList[k].write_protected;
                        pte->hole = 1;
                        break;
                    }
                }
            }

            if (!validAddr)
            {
                //segmentation fault

                if (O_option)
                {
                    cout << " SEGV" << endl;
                    total_cost2 += 240;
                }
                current_process->segv++;
                //moveon
                continue;
            }

            FTE *newframe = get_frame();

            if (newframe->victimed) //this means we are dealing with a victimed frame not fresh frame
            {
                int reverse_map_vpage = newframe->vpage;
                int reverse_map_procid = newframe->procID;

                PTE *p = &processes[reverse_map_procid].pageTable[reverse_map_vpage];

                p->present = 0;
                processes[reverse_map_procid].unmaps++;

                //unmap over
                total_cost2 += 400;
                if (O_option)
                    cout << " UNMAP " << reverse_map_procid << ":" << reverse_map_vpage << endl;

                if (p->modified)
                {
                    if (p->filemap)
                    {
                        processes[reverse_map_procid].fouts++;
                        p->modified = false;
                        total_cost2 += 2500;
                        if (O_option)
                            cout << " FOUT" << endl;
                    }
                    else
                    {
                        processes[reverse_map_procid].pageouts++;
                        p->modified = false;
                        p->pagedout = true;
                        total_cost2 += 3000;
                        if (O_option)
                            cout << " OUT" << endl;
                    }

                    p->frame_number = 0;
                }
            }

            //bot bak

            if (pte->filemap)
            {
                if (O_option)
                    cout << " FIN" << endl;
                current_process->fins++;
                total_cost2 += 2500;
            }

            else
            {
                if (pte->pagedout)
                {
                    if (O_option)
                        cout << " IN" << endl;
                    total_cost2 += 3000;
                    current_process->pageins++;
                }
                else
                {
                    if (O_option)
                        cout << " ZERO" << endl;
                    current_process->zeros++;
                    total_cost2 += 150;
                }
            }

            newframe->procID = current_process->pid;
            newframe->vpage = cur_vpage;
            newframe->age = 0;
            newframe->tau = ins_counter-1;
            if (O_option)
                cout << " MAP " << newframe->frameNum << endl;

            pte->present = 1;
            pte->frame_number = newframe->frameNum;
            total_cost2 += 400;
            current_process->maps++;
        }
        //modified if wr
        if (currentIns.instr == 'r' || currentIns.instr == 'w')
        {
            rwaccesses++;
            pte->referenced = 1;
            
        }

        if (currentIns.instr == 'w')
        {
            if (pte->write_protect)
            {
                if (O_option)
                    cout << " SEGPROT" << endl;
                current_process->segprot++;
                total_cost2 += 300;
            }
            else
            {
                pte->modified = 1;
            }
        }
    }
}

// void costCalc()
// {

//     //adding one by one for correct calc
//     for (int i = 0; i < processes.size(); i++)
//     {
//         Process *proc = &processes[i];
//         //adding maps
//         for (int i = 0; i < proc->maps; i++)
//         {
//             total_cost += (unsigned long long)400;
//         }
//         //adding unmaps
//         for (int i = 0; i < proc->unmaps; i++)
//         {
//             total_cost += (unsigned long long)400;
//         }
//         //adding pageins
//         for (int i = 0; i < proc->pageins; i++)
//         {
//             total_cost += (unsigned long long)3000;
//         }
//         //pageouts
//         for (int i = 0; i < proc->pageouts; i++)
//         {
//             total_cost += (unsigned long long)3000;
//         }
//         //fins
//         for (int i = 0; i < proc->fins; i++)
//         {
//             total_cost += (unsigned long long)2500;
//         }
//         //fouts
//         for (int i = 0; i < proc->fouts; i++)
//         {
//             total_cost += (unsigned long long)2500;
//         }
//         //zero
//         for (int i = 0; i < proc->zeros; i++)
//         {
//             total_cost += (unsigned long long)150;
//         }
//         for (int i = 0; i < proc->segv; i++)
//         {
//             total_cost += (unsigned long long)240;
//         }
//         for (int i = 0; i < proc->segprot; i++)
//         {
//             total_cost += (unsigned long long)300;
//         }
//     }

//     total_cost += (unsigned long long)rwaccesses;

//     for (int i = 0; i < total_context_switches; i++)
//         total_cost += (unsigned long long)121;
//     for (int i = 0; i < total_proc_exits; i++)
//         total_cost += (unsigned long long)121;
// }

void finalOutput()
{
    if (P_option)
        for (int i = 0; i < processes.size(); i++)
        {
            printPageTable(i);
        }
    if (F_option)
        printFrameTable();
    if (S_option)
    {
        for (int i = 0; i < processes.size(); i++)
        {
            printf("PROC[%d]: U=%llu M=%llu I=%llu O=%llu FI=%llu FO=%llu Z=%llu SV=%llu SP=%llu\n", processes[i].pid,
                   processes[i].unmaps, processes[i].maps, processes[i].pageins, processes[i].pageouts, processes[i].fins, processes[i].fouts, processes[i].zeros, processes[i].segv, processes[i].segprot);
        }

        //costCalc();
        printf("TOTALCOST %lu %llu %llu %llu\n", instructions.size(), total_context_switches, total_proc_exits, total_cost2);
    }
}

int main(int argc, char **argv)
{
    int c;
    char *argPtr;
    while ((c = getopt(argc, argv, "f:a:o:")) != -1)
    {
        switch (c)
        {
        case 'f':
            num_frames = atoi(optarg);
            break;
        case 'a':
            switch (optarg[0])
            {
            case 'f':
                algChoice = 0; //fifo
                break;
            case 'r':
                algChoice = 1; //random
                break;
            case 'c':
                algChoice = 2; //clock
                break;
            case 'e':
                algChoice = 3; //enhanced Second Chance NRU
                break;
            case 'a':
                algChoice = 4; //aging
                break;
            case 'w':
                algChoice = 5; //working set
                break;
            default:
                break;
            }
            break;
        case 'o':
            argPtr = optarg;
            while (*argPtr != '\0')
            {
                switch (*argPtr)
                {
                case 'O':
                    O_option = true;
                    break;
                case 'P':
                    P_option = true;
                    break;
                case 'F':
                    F_option = true;
                    break;
                case 'S':
                    S_option = true;
                    break;
                case 'x':
                    x_option = true;
                    break;
                case 'y':
                    y_option = true;
                    break;
                case 'f':
                    f_option = true;
                    break;
                case 'a':
                    a_option = true;
                    break;
                default:
                    break;
                }
                argPtr++;
            }
            break;
        default:
            break;
        }
    }

    string inp_file = argv[optind];
    string ran_file = argv[optind + 1];

    reader(inp_file, ran_file); //call to reader to read files and instruction
    frameTableinit(); // init frame table
    simulator(); //call to simulation to sim the required algorithm
    finalOutput(); //final outputs
    return 0;
}
