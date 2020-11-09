#include<iostream>  
#include<iomanip>
#include<fstream>
#include<vector>
#include<ctype.h>
#include<string>
#include<string.h>
#include<algorithm>
using namespace std; 


int moduleSize[100];

class Token 
{
    public:
    int lineNumber;
    int lineOffset;
    string tokenContents;

    void setToken(int lineNo, int lineOff, string tokenStr)
    {
        lineNumber = lineNo;
        lineOffset = lineOff;
        tokenContents = tokenStr;
    }
};

class Symbol
{
    public:
    string sym;
    int Addr;
    int moduleNum;
    bool alreadyDefined = false;
    bool used = false;

};


void __parseerror(int errcode, Token tokenVar) 
{
	string errstr[] = {
		"NUM_EXPECTED",
		"SYM_EXPECTED",
		"ADDR_EXPECTED",
		"SYM_TOO_LONG",
		"TOO_MANY_DEF_IN_MODULE",
		"TOO_MANY_USE_IN_MODULE",
		"TOO_MANY_INSTR",
	};
    cout<<"Parse Error line "<<tokenVar.lineNumber<<" offset "<<tokenVar.lineOffset<<": "<<errstr[errcode]<<endl; 
	exit(0);
}

int readInt(Token tokenVar) 
{   
    string s = tokenVar.tokenContents;
    int i;
	try
	{
		 i = std::stoi(s);
		
	}
	catch (std::invalid_argument const &e)
	{
		__parseerror(0, tokenVar);
	}
	catch (std::out_of_range const &e)
	{
		__parseerror(0, tokenVar);
	}

	return i;
}

string readSymbol(Token tokenVar)
{
    string s = tokenVar.tokenContents;
    int i = 0;
    if(strlen(&s[0])<1)
    {
        __parseerror(1,tokenVar);
    }
    while(s[i])
    {
        if(i == 0)
        {
            if(!isalpha(s[i]))
            __parseerror(1,tokenVar);
        }

        if(!isalnum(s[i]))
        {
            __parseerror(1,tokenVar);
        }

        i++;
    }
    char* k = &s[0];
    if(strlen(k)>16)
    {
        __parseerror(3,tokenVar);
    }
    return s;

}

string readIAER(Token tokenVar)
{
    string s = tokenVar.tokenContents;
    
    if(s.length()>1)
    {   
        __parseerror(2,tokenVar);
    }
    if(s[0]!='I'&&s[0]!='A'&&s[0]!='E'&&s[0]!='R')
    {   
        
        __parseerror(2,tokenVar);
    }

    return s;
}


vector<Symbol> parse1(string fileName)
{
    
    ifstream file(fileName);
    string line;
    int temp;
    int lineNo = 1;
    char cArray[100];
    int totalInst = 0;

    vector<Token> tokens; 

    if(!file)
    {
        cout<<"Unable to open file "<<fileName<<endl;
        exit(0);
    }
    int ll;
    vector<Symbol> symbol;
    
    while(getline(file,line))
    {   
        //getting tokens
        ll = strlen(&line[0]);
        int lineOffset = 1;
        char* str = &line[0];
        char* tok;
        char* remainingLine;
        tok = strtok (str," \t");
        while (tok != NULL)
        {   
            //cout<<"Token is : "<<tok<<" found at positon  "<<lineOffset<<" in line "<<lineNo<<endl;
            int curTokLength = strlen(tok);
            Token t;
            t.setToken(lineNo,lineOffset,tok);

            tokens.push_back(t);
            
            remainingLine = strtok (NULL,"");
            tok = strtok (remainingLine, " \t");
            if(tok!=NULL)
            {
                char * pch;
                pch = strstr (remainingLine,tok);
                lineOffset = lineOffset + curTokLength + strlen(remainingLine) - strlen(pch) + 1;
            }   
        }
        lineNo++;
    }
    Token last;
    if(lineNo == 1 && ll ==0) last.setToken(1,1,"");
    else last.setToken(lineNo-1,ll+1,"");
    tokens.push_back(last);

    int ct = 0;
    int moduleNo = 0; //current module number 
    int curBaseAddress = 0;
    
    //parsing 
    while(ct<tokens.size()-1)
    {   
        moduleNo++;
        
        Token curTok;
        int defcount;
        try
        {   
            curTok = tokens.at(ct);   
                  
            defcount = readInt(curTok);
            //cout<<curTok.tokenContents; 
            ct++;
        }
        catch(exception E)
        {
            __parseerror(0, last);
        }

        if(defcount > 16)
        {
            __parseerror(4,curTok);
        }    
        //creating symbols
        for(int p = 0;p<defcount;p++)
        {   
            Symbol s;
            
            
            try
            {
                Token sym = tokens.at(ct);
                
                string k = readSymbol(sym);
                
                s.sym =k;
                ct++;

            }
            catch(exception E)
            {
                __parseerror(1, last);
            }

            try
            {
               Token symAddr = tokens.at(ct);
               int addr = readInt(symAddr)+curBaseAddress;
               s.Addr = addr;
               s.moduleNum = moduleNo;
               ct++;
            }
            catch(exception E)
            {
                __parseerror(0, last);   
            }
            int flag = 0;
            for(int d = 0;d<symbol.size();d++)
            {
                if(symbol[d].sym.compare(s.sym)==0)
                {
                    symbol[d].alreadyDefined = true;
                    flag = 1;
                }
            }
            if(flag == 0)
            {
                symbol.push_back(s);
            }
            
            
            
        }
        //creating symbols over
        
        int usecount;
        try
        {
            curTok = tokens.at(ct);
            usecount = readInt(curTok);
            ct++;
        }
        catch(exception E)
        {
            __parseerror(0, last);
        }

        if(usecount > 16)
        {
            __parseerror(5,curTok);
        }
        

        for(int p=0;p<usecount;p++)
        {
            try
            {
                Token sym = tokens.at(ct);
                string k = readSymbol(sym);
                ct++;
            }
            catch(exception E)
            {
                __parseerror(1, last);
            }

        }
        
        //usecount over

        int instcount;
        
        Token insTok;
        try
        {
            insTok = tokens.at(ct);

            instcount = readInt(insTok);
            totalInst = totalInst+instcount;
            ct++;

        }
        
        catch(const std::exception& e)
        {
            __parseerror(0,last);
        }
        if(totalInst>512)
        {
            __parseerror(6,insTok);
        }
        for(int k=0;k<instcount;k++)
        {   
            string addressMode;
            int addr;
            try
            {
                Token aMode = tokens.at(ct);
                addressMode = readIAER(aMode); 
                ct++;   
            }
            catch(const std::exception& e)
            {
                __parseerror(2,last);
            }
            
            try
            {
                Token addrs = tokens.at(ct);
                addr = readInt(addrs);

                ct++;
            }
            catch(const std::exception& e)
            {
                __parseerror(0,last);
            }
            
            
            //checks done here
        }
        curBaseAddress = curBaseAddress + instcount;
        moduleSize[moduleNo]=curBaseAddress;
        
    }

    file.close();
    return symbol;

    
}

vector<Symbol> parse2(string fileName,vector <Symbol> symTable)
{
    
    ifstream file(fileName);
    string line;
    int temp;
    int lineNo = 1;
    char cArray[100];

    vector<Token> tokens; 

    if(!file)
    {
        cout<<"Unable to open file "<<fileName<<endl;
        exit(0);
    }
    int ll;
    vector<Symbol> symbol;
    while(getline(file,line))
    {   
        //getting tokens
        ll = strlen(&line[0]);
        int lineOffset = 1;
        char* str = &line[0];
        char* tok;
        char* remainingLine;
        tok = strtok (str," \t");
        
        while (tok != NULL)
        {   
            //cout<<"Token is : "<<tok<<" found at positon  "<<lineOffset<<" in line "<<lineNo<<endl;
            int curTokLength = strlen(tok);
            Token t;
            t.setToken(lineNo,lineOffset,tok);

            tokens.push_back(t);
            
            remainingLine = strtok (NULL,"");
            tok = strtok (remainingLine, " \t");
            if(tok!=NULL)
            {
                char * pch;
                pch = strstr (remainingLine,tok);
                lineOffset = lineOffset + curTokLength + strlen(remainingLine) - strlen(pch) + 1;
            }   
        }
        lineNo++;
    }
    Token last;
    if(lineNo == 1 && ll ==0) last.setToken(1,1,"");
    else last.setToken(lineNo-1,ll+1,"");
    tokens.push_back(last);


    int ct = 0;
    int moduleNo = 0; //current module number 
    int curBaseAddress = 0;
    int memoryMap = 0;
    //parsing 
    while(ct<tokens.size()-1)
    {   
        moduleNo++;
        Token curTok;
        int defcount;
        cout<<curTok.tokenContents;
        try
        {   
            curTok = tokens.at(ct);          
            defcount = readInt(curTok);
            ct++;
        }
        catch(exception E)
        {
            __parseerror(0, last);
        }
 
        ct = ct+2*defcount;
        
        int usecount;
        try
        {
            curTok = tokens.at(ct);
            usecount = readInt(curTok);
            ct++;
        }
        catch(exception E)
        {
            __parseerror(0, last);
        }
        vector<string> useCountSyms;
        vector<int> RefferedInE; 
        for(int p=0;p<usecount;p++)
        {
            try
            {
                Token sym = tokens.at(ct);
                string k = readSymbol(sym);
                useCountSyms.push_back(k);
                for(int e =0;e<symTable.size();e++)
                {
                    if(symTable[e].sym.compare(k)==0)
                    {
                        symTable[e].used=true;
                    }
                }
                ct++;
            }
            catch(exception E)
            {
                __parseerror(1, last);
            }

        }
        
        //usecount over

        int instcount;
        Token insTok;
        try
        {
            insTok = tokens.at(ct);
            instcount = readInt(insTok);
            ct++;
        }
        
        catch(const std::exception& e)
        {
            __parseerror(0,last);
        }
        if(instcount>512)
        {
            __parseerror(6,insTok);
        }
        int memory = 0;
        bool errorS = false;
        string errstr = "";
        for(int k=0;k<instcount;k++)
        {   
            string addressMode;
            int addr;
            try
            {
                Token aMode = tokens.at(ct);
                addressMode = readIAER(aMode); 
                ct++;   
            }
            catch(const std::exception& e)
            {
                __parseerror(2,last);
            }
            
            try
            {
                Token addrs = tokens.at(ct);
                addr = readInt(addrs);
                //cout<<"addr"<<addr;
                ct++;
            }
            catch(const std::exception& e)
            {
                __parseerror(0,last);
            }
            bool opcodeErr = false;
            
            int opcode = addr/1000;

            
            int operand = addr%1000;
            int outputAddr = 0;
            if(addr>9999)
            {
                
                opcodeErr = true;
                errstr = "Error: Illegal opcode; treated as 9999";
                opcode = 9;
                operand = 999;
                outputAddr = 9999;
                
                
            }

            else if(addressMode[0]=='R')
            {
                opcode = addr/1000;
                operand = addr%1000;
                
                if(operand<instcount)
                    outputAddr = curBaseAddress + opcode*1000 + operand;
                else
                {
                    outputAddr = curBaseAddress + opcode*1000;
                    errstr = "Error: Relative address exceeds module size; zero used";
                    errorS = true;
                }
                

            }
            else;
            if (addressMode[0]=='I')
            {
                if(addr>9999)
                {
                    addr = 9999;
                    errstr = "Error: Illegal immediate value; treated as 9999";
                    errorS = true;
                }
                outputAddr = addr;
            }
            else if (addressMode[0]=='E')
            {
                string useSym;
                try
                {   
                    useSym = useCountSyms.at(operand);
                    RefferedInE.push_back(operand);
                    //cout<<useSym;
                    bool notfound = true;
                    for(int u = 0;u<symTable.size();u++)
                    {
                        Symbol S = symTable[u];
                        if(S.sym.compare(useSym)==0)
                        {
                            
                            int opc = opcode;
                            int operand = S.Addr;
                            outputAddr = (opc*1000)+operand;
                            notfound = false;
                            break;
                        } 


                    }
                        if(notfound)
                        {
                            errstr="Error: "+useSym+ " is not defined; zero used";
                            errorS = true;
                            outputAddr = opcode*1000+0;
                        }
                }
                catch(const std::exception& e)
                {
                    errstr = "Error: External address exceeds length of uselist; treated as immediate";
                    errorS = true;
                    outputAddr = addr;
                    
                }

            }
            else if (addressMode[0]=='A')
            {
                if(operand<512)
                {
                    outputAddr = addr;
                }
                else
                {
                    errstr = "Error: Absolute address exceeds machine size; zero used";
                    outputAddr = opcode*1000;
                    errorS = true;
                }
                
            }
            else;
            
            
            cout<<setfill('0')<<setw(3)<<memoryMap;
            memoryMap++;
            cout<<":"<<" "<<setfill('0')<<setw(4)<<outputAddr;
            if(opcodeErr)
            {
                cout<<" "<<errstr;
            }
            else if(errorS)
            {
                cout<<" "<<errstr;
            }
            
            cout<<endl;
            errstr = "";
            errorS = false;

            
            //checks done here
        }
        curBaseAddress = curBaseAddress + instcount;


        for(int g = 0;g<useCountSyms.size();g++)
        {
            bool found = false;
            for(int n=0;n<RefferedInE.size();n++)
            {
                if(g==RefferedInE[n])
                {
                    found = true;
                }
            }
            if(!found)
            cout<<"Warning: Module "<<moduleNo<<": "<<useCountSyms[g]<<" appeared in the uselist but was not actually used\n";
		    
        }




    }

    return symTable;
    
}



int main(int argc, char** argv) 
{ 
    //check inputs format:
    if(argc!=2)
    {
        cout<<" Wrong Inputs \n";
        exit(0);
    }

    string fileName = argv[1];
    
    //calling parse 1

    vector<Symbol> symboltable = parse1(fileName);


        for(int y=0;y<symboltable.size();y++)
    {   
        //cout<<symboltable[y].moduleNum<<" is modulenum";
        if(symboltable[y].Addr>moduleSize[symboltable[y].moduleNum]-1)
        {
            cout<<"Warning: Module "<<symboltable[y].moduleNum<<": "<<symboltable[y].sym<<" too big "<<symboltable[y].Addr<<" (max="<<moduleSize[symboltable[y].moduleNum]-1<<") assume zero relative\n";
            symboltable[y].Addr=symboltable[y].Addr/1000;
            symboltable[y].Addr=symboltable[y].Addr*1000;
        }
    }


    cout<<"Symbol Table"<<endl;

    for(int i =0;i<symboltable.size();i++)
    {
        cout<<symboltable[i].sym<<"="<<symboltable[i].Addr;
        if(symboltable[i].alreadyDefined)
        {
            cout<<" Error: This variable is multiple times defined; first value used";
        }
        cout<<endl;
    }
    


    cout<<endl<<"Memory Map"<<endl;



    //calling parse 2 and passing symbol table as parameter. Tokens not stored.

    symboltable = parse2(fileName,symboltable);



    cout<<endl;

    for(int e =0;e<symboltable.size();e++)
    {
        if(symboltable[e].used==false)
        {
            cout<<"Warning: Module "<<symboltable[e].moduleNum<<": "<<symboltable[e].sym<<" was defined but never used"<<endl;
        }
    }    
    
    
    return 0;
} 

