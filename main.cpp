//islam wagih emam 20190099
//bassant samer mahmoud 20190133

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>

using namespace std;
/*
{ Sample program
  in TINY language
  compute factorial
}

read x; {input an integer}
if 0<x then {compute only if x>=1}
  fact:=1;
  repeat
    fact := fact * x;
    x:=x-1
  until x=0;
  write fact {output factorial}
end
*/

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// variable names can include only alphabetic charachters(a:z or A:Z) and underscores
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n=0)
{
    if(n>0) {strncpy(a, b, n); a[n]=0;}
    else strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b==0) {*a=0; return;}
    int n=strlen(b);
    *a=new char[n+1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str) {file=0; if(str) file=fopen(str, "r"); cur_line_size=0; cur_ind=0; cur_line_num=0;}
    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind<cur_line_size)
        {
            char ch=line_buf[cur_ind];
            if(ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind>=cur_line_size) {if(!GetNewLine()) return false; SkipSpaces();}

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind=0; line_buf[0]=0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size=strlen(line_buf);
        if(cur_line_size==0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
        SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; SkipSpaces();}
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind+=num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s, bool newLine=true)
    {
        if(newLine){fprintf(file, "%s\n", s);}
        else{fprintf(file, "%s", s);}
        fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char* in_str, const char* out_str, const char* debug_str)
                : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN,
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                ENDFILE, ERROR
              };

// Used for debugging only /////////////////////////////////////////////////////////
const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
            };

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

const Token reserved_words[]=
{
    Token(IF, "if"),
    Token(THEN, "then"),
    Token(ELSE, "else"),
    Token(END, "end"),
    Token(REPEAT, "repeat"),
    Token(UNTIL, "until"),
    Token(READ, "read"),
    Token(WRITE, "write")
};
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(LESS_THAN, "<"),
    Token(PLUS, "+"),
    Token(MINUS, "-"),
    Token(TIMES, "*"),
    Token(DIVIDE, "/"),
    Token(POWER, "^"),
    Token(SEMI_COLON, ";"),
    Token(LEFT_PAREN, "("),
    Token(RIGHT_PAREN, ")"),
    Token(LEFT_BRACE, "{"),
    Token(RIGHT_BRACE, "}")
};

const int num_symbolic_tokens=sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}
inline bool IsWhiteSpace(char ch){return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';}

//return the right token for the symbol
TokenType symbolToken(char x)
{
    switch(x)
    {
        case '{': return LEFT_BRACE;
        case '}': return RIGHT_BRACE;
        case '(': return LEFT_PAREN;
        case ')': return RIGHT_PAREN;
        case '*': return TIMES;
        case '^': return POWER;
        case '+': return PLUS;
        case '-': return MINUS;
        case '/': return DIVIDE;
        case '=': return EQUAL;
        case ':': return ASSIGN;
        case '<': return LESS_THAN;
        case ';': return SEMI_COLON;
        default: return ERROR;
    }
}

void lineNumber(OutFile* output, InFile* input)
{
    output->Out("[", false);
    char number[MAX_TOKEN_LEN];
    itoa(input->cur_line_num, number, 10);
    output->Out(number, false);
    output->Out("] ", false);
}

struct ScanNode
{
    char* info; //code
    char* tokenTypeStr; //token type
    int lineNumber; //line number corresponding to code file
    ScanNode(const char* info, const char* tokenTypeStr, int lineNumber)
    {
        //reserve memory
        this->info = new char[MAX_TOKEN_LEN];
        this->tokenTypeStr = new char[MAX_TOKEN_LEN];
        this->lineNumber = lineNumber;
        Copy(this->info, info);
        Copy(this->tokenTypeStr, tokenTypeStr);
    }
    //destructor to release the memory
    ~ScanNode()
    {
        delete[] info;
        delete[] tokenTypeStr;
    }
};

//scanning function
vector<ScanNode*> scanner(const char* inputFile, const char* outputFile)
{
    InFile file(inputFile);
    OutFile output(outputFile);

    vector<ScanNode*> scanNodes;

    while(file.GetNewLine())
    {
        //current line
        char* tokenLine = file.line_buf;
        //iterate over current line
        int i = 0, leftBrace = 0;
        while(i < file.cur_line_size)
        {

            char* tokenName = new char[MAX_TOKEN_LEN+1];
            int currInd = 0;

            //digit[0-9]
            while(IsDigit(tokenLine[i]))
            {
                tokenName[currInd++] = tokenLine[i++];
            }

            //we have constructed number(Num)token
            if(currInd != 0)
            {
                //null character
                tokenName[currInd] = 0;
                lineNumber(&output, &file);
                output.Out(tokenName, false);
                output.Out(" (", false);
                output.Out(TokenTypeStr[NUM], false);
                output.Out(")");
                scanNodes.push_back(new ScanNode(tokenName, TokenTypeStr[NUM], file.cur_line_num));
                continue;
            }

            //char[a-z][A-Z]-underscore
            while(IsLetterOrUnderscore(tokenLine[i]))
            {
                tokenName[currInd++] = tokenLine[i++];
            }

            //we have constructed identifier or reserved word token
            if(currInd != 0)
            {
                //null character
                tokenName[currInd] = 0;
                lineNumber(&output, &file);
                output.Out(tokenName, false);
                output.Out(" (", false);
                //search if it is a reserved word
                bool isReservedWord = false;
                for(int j=0;j<num_reserved_words;j++)
                {
                    if(Equals(tokenName, reserved_words[j].str))
                    {
                        isReservedWord = true;
                        output.Out(TokenTypeStr[j], false);
                        scanNodes.push_back(new ScanNode(tokenName, TokenTypeStr[j], file.cur_line_num));
                    }
                }
                //if not a reserved word then it's an identifier
                if(!isReservedWord)
                {
                    output.Out("ID", false);
                    scanNodes.push_back(new ScanNode(tokenName, "ID", file.cur_line_num));
                }
                output.Out(")");
                continue;
            }

            //defined or undefined symbol or space
            if(!IsWhiteSpace(tokenLine[i]))
            {
                tokenName[currInd++] = tokenLine[i++];
            }

            //we have constructed a symbolic token
            if(currInd != 0)
            {
                TokenType type = symbolToken(tokenLine[i-1]);
                if(type == ASSIGN)
                {
                    if(i == file.cur_line_size || tokenLine[i] != '=') {type = ERROR;}
                    else{tokenName[currInd++] = '=';i++;} //skip equal sign that is part of := symbol
                }else if(type == LEFT_BRACE)
                {

                    //if we found a left brace we must continue until we found a right brace
                    //we need to keep a counter for LEFT_BRACE we have seen to handle nested braces
                    leftBrace++;
                }

                //null character
                tokenName[currInd] = 0;
                lineNumber(&output, &file);
                output.Out(tokenName, false);
                output.Out(" (", false);
                output.Out(TokenTypeStr[type], false);
                output.Out(")");
                if(type != LEFT_BRACE && type != RIGHT_BRACE) scanNodes.push_back(new ScanNode(tokenName, TokenTypeStr[type], file.cur_line_num));
            }else
            {
                //skip any space
                i++;
            }
            //handle comments
            while(i < file.cur_line_size && leftBrace)
            {
                if(tokenLine[i] == '}') leftBrace--;
                else if(tokenLine[i] == '{') leftBrace++;
                if(leftBrace > 0) i++;
                if(i == file.cur_line_size)
                {
                    bool hasNewline = file.GetNewLine();
                    if(!hasNewline) break;
                    tokenLine = file.line_buf;
                    i = 0;
                }
            }
        }
    }

    //token of endOfFile
    file.cur_line_num++;
    lineNumber(&output, &file);
    output.Out("EOF", false);
    output.Out(" (", false);
    output.Out(TokenTypeStr[ENDFILE], false);
    output.Out(")");
    scanNodes.push_back(new ScanNode("EOF", TokenTypeStr[ENDFILE], file.cur_line_num));
    return scanNodes;
}


////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

// program -> stmtseq
// stmtseq -> stmt { ; stmt }
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
// ifstmt -> if expr then stmtseq [ else stmtseq ] end
// repeatstmt -> repeat stmtseq until expr
// assignstmt -> identifier := expr
// readstmt -> read identifier
// writestmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> factor { (*|/) factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier

enum NodeKind{
                IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
                OPER_NODE, NUM_NODE, ID_NODE
             };

// Used for debugging only /////////////////////////////////////////////////////////
const char* NodeKindStr[]=
            {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID"
            };

enum ExprDataType {VOID, INTEGER, BOOLEAN};

// Used for debugging only /////////////////////////////////////////////////////////
const char* ExprDataTypeStr[]=
            {
                "Void", "Integer", "Boolean"
            };

#define MAX_CHILDREN 3

struct TreeNode
{
    TreeNode* child[MAX_CHILDREN];
    TreeNode* sibling; // used for sibling statements only

    NodeKind node_kind;

    union{TokenType oper; int num; char* id;}; // defined for expression/int/identifier only
    ExprDataType expr_data_type; // defined for expression/int/identifier only

    int line_num;

    TreeNode() {int i; for(i=0;i<MAX_CHILDREN;i++) child[i]=0; sibling=0; expr_data_type=VOID;}
};

void PrintTree(TreeNode* node, int sh=0)
{
    int i, NSH=3;
    for(i=0;i<sh;i++) printf(" ");

    printf("[%s]", NodeKindStr[node->node_kind]);

    if(node->node_kind==OPER_NODE) printf("[%s]", TokenTypeStr[node->oper]);
    else if(node->node_kind==NUM_NODE) printf("[%d]", node->num);
    else if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE) printf("[%s]", node->id);

    if(node->expr_data_type!=VOID) printf("[%s]", ExprDataTypeStr[node->expr_data_type]);

    printf("\n");

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) PrintTree(node->child[i], sh+NSH);
    if(node->sibling) PrintTree(node->sibling, sh);
}

struct tokenIndex
{
    int value;
};

//header to use the mathexpr function inside newexpr function
TreeNode* mathexpr(vector<ScanNode*>& tokens, tokenIndex* tokenInd);

// newexpr -> ( mathexpr ) | number | identifier
TreeNode* newexpr(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //check if we have at least one token
    if(tokenInd->value == tokens.size())
    {
        return nullptr;
    }
    //newexpr expanded to number
    if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[NUM]))
    {
        TreeNode* node = new TreeNode;
        //convert string to integer
        node->num = stoi(tokens[tokenInd->value]->info);
        node->node_kind = NUM_NODE;
        //go to next token
        tokenInd->value++;
        return node;
    }

    //newexpr expanded to identifier
    if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ID]))
    {
        TreeNode* node = new TreeNode;
        //reserve memory for id string
        node->id = new char[strlen(tokens[tokenInd->value]->info)+1];
        node->node_kind = ID_NODE;
        Copy(node->id, tokens[tokenInd->value]->info);
        //go to next token
        tokenInd->value++;
        return node;
    }

    //newexpr expanded to ( mathexpr )
    if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[LEFT_PAREN]))
    {
        //move to next token
        tokenInd->value++;
        //if there is no next token then return error
        if(tokenInd->value == tokens.size())
        {
            cout<<"missing statement after open parentheses '(' at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
            return nullptr;
        }
        //expand the mathpexr
        TreeNode* node = mathexpr(tokens, tokenInd);
        //if no more tokens or next one is not right paren then return nullptr
        if(tokenInd->value == tokens.size() || !Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[RIGHT_PAREN]))
        {
            cout<<"missing closing parentheses ')' at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
            return nullptr;
        }
        //go to next token
        tokenInd->value++;
        return node;
    }

    //if we can't expanded then return an error
    return nullptr;
}

// factor -> newexpr { ^ newexpr }    right associative
TreeNode* factor(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    TreeNode* node = newexpr(tokens, tokenInd);
    //if we have no more tokens or can't do newexper then return node as it is
    if(tokenInd->value == tokens.size() || node == nullptr)
    {
        return node;
    }
    //see if next token is power
    if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[POWER]))
    {
        //        ^
        //newexpr   newexpr
        TreeNode* parent = new TreeNode;
        parent->node_kind = OPER_NODE;
        parent->oper = POWER;
        parent->child[0] = node;
        tokenInd->value++;
        parent->child[1] = factor(tokens, tokenInd);
        return parent;
    }
    return node;
}


// term -> factor { (*|/) factor }    left associative
TreeNode* term(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    TreeNode* node = factor(tokens, tokenInd);
    while
    (
        Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[DIVIDE]) ||
        Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[TIMES])
    )
    {
        TokenType type = Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[DIVIDE]) ? DIVIDE:TIMES;
        TreeNode* parent = new TreeNode;
        parent->node_kind = OPER_NODE;
        parent->oper = type;
        parent->child[0] = node;
        tokenInd->value++;
        parent->child[1] = factor(tokens, tokenInd);
        node = parent;
    }
    return node;
}

// mathexpr -> term { (+|-) term }    left associative
TreeNode* mathexpr(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    TreeNode* node = term(tokens, tokenInd);
    while
    (
        Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[MINUS]) ||
        Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[PLUS])
    )
    {
        TreeNode* parent = new TreeNode;
        TokenType type = Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[MINUS]) ? MINUS:PLUS;
        parent->node_kind = OPER_NODE;
        parent->oper = type;
        parent->child[0] = node;
        tokenInd->value++;
        parent->child[1] = term(tokens, tokenInd);
        node = parent;
    }
    return node;
}

// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* expr(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    TreeNode* node = mathexpr(tokens, tokenInd);
    char* str = tokens[tokenInd->value]->tokenTypeStr;
    if(Equals(str, TokenTypeStr[LESS_THAN]) || Equals(str, TokenTypeStr[EQUAL]))
    {
        TreeNode* parent = new TreeNode;
        parent->child[0] = node;
        parent->node_kind = OPER_NODE;
        TokenType type = Equals(str, TokenTypeStr[LESS_THAN]) ? LESS_THAN:EQUAL;
        parent->oper = type;
        tokenInd->value++;
        parent->child[1] = expr(tokens, tokenInd);
        return parent;
    }
    return node;
}

// writestmt -> write expr
TreeNode* writestmt(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //if i did not found write statement then error
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[WRITE]))
    {
        return nullptr;
    }
    //define the node then do the expression
    TreeNode* node = new TreeNode;
    node->node_kind = WRITE_NODE;
    //go to next token
    tokenInd->value++;
    node->child[0] = expr(tokens, tokenInd);
    return node;
}

// readstmt -> read identifier
TreeNode* readstmt(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //if i did not found an read statement
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[READ]))
    {
        return nullptr;
    }
    tokenInd->value++;
    //if i did not found identifier then error
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ID]))
    {
        cout<<"read statement without an identifier at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
        return nullptr;
    }
    //allocate then go to next token
    TreeNode* node = new TreeNode;
    node->node_kind = READ_NODE;
    node->id = new char[strlen(tokens[tokenInd->value]->info)+1];
    Copy(node->id, tokens[tokenInd->value]->info);
    tokenInd->value++;
    return node;

}
// assignstmt -> identifier := expr
TreeNode* assignstmt(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //can't match identifier
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ID]))
    {
        return nullptr;
    }
    //allocate the node
    TreeNode* node = new TreeNode;
    node->node_kind = ASSIGN_NODE;
    node->id =  new char(strlen(tokens[tokenInd->value]->info)+1);
    Copy(node->id, tokens[tokenInd->value]->info);
    //move to next token it should be an assign operator
    tokenInd->value++;
    //can't match assign operator
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ASSIGN]))
    {
        cout<<"missing assign operator := in assignment statment at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
        return nullptr;
    }
    //matched the assign operator go to expr token
    tokenInd->value++;
    node->child[0] = expr(tokens, tokenInd);
    return node;
}

// handle stmtseq definition
TreeNode* stmtseq(vector<ScanNode*>& tokens, tokenIndex* tokenInd);

// repeatstmt -> repeat stmtseq until expr
TreeNode* repeatstmt(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //can't match with repeat stmt
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[REPEAT]))
    {
        return nullptr;
    }
    tokenInd->value++;
    TreeNode* node = new TreeNode;
    node->node_kind = REPEAT_NODE;
    //expand stmtseq
    node->child[0] = stmtseq(tokens, tokenInd);
    //can't match with until stmt
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[UNTIL]))
    {
        cout<<"missing until in repeat statement at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
        return nullptr;
    }
    //skip until token
    tokenInd->value++;
    //expand expr
    node->child[1] = expr(tokens, tokenInd);
    return node;
}

// ifstmt -> if expr then stmtseq [ else stmtseq ] end
TreeNode* ifstmt(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //can't match with if reserved word
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[IF]))
    {
        cout<<"can't match with if\n";
        return nullptr;
    }
    //go to next token
    tokenInd->value++;
    TreeNode* node = new TreeNode;
    node->node_kind = IF_NODE;
    //expand expr
    node->child[0] = expr(tokens, tokenInd);
    //can't match with then reserved word
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[THEN]))
    {
        cout<<"missing then in if statement at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
        return nullptr;
    }
    tokenInd->value++;
    //expand stmtseq
    node->child[1] = stmtseq(tokens, tokenInd);
    //check if there is else token
    if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ELSE]))
    {
        //skip else token
        tokenInd->value++;
        node->child[2] = stmtseq(tokens, tokenInd);
    }
    //can't match with end reserved word
    if(!Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[END]))
    {
        cout<<"missing end in if statement at line "<<tokens[tokenInd->value]->lineNumber<<'\n';
        return nullptr;
    }
    //go to next token skip end
    tokenInd->value++;
    return node;
}

// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
TreeNode* stmt(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    // test the current token
    TreeNode* node = nullptr;
    char* str = tokens[tokenInd->value]->tokenTypeStr;
    //cout<<"current to match "<<str<<endl;
    //try to expand the noed
    if(Equals(str, TokenTypeStr[IF]))
    {
        //cout<<"before ifstmt\n";
        node = ifstmt(tokens, tokenInd);
        //cout<<"after ifstmt\n";
    }else if(Equals(str, TokenTypeStr[REPEAT]))
    {
        //cout<<"before repeatstmt\n";
        node = repeatstmt(tokens, tokenInd);
        //cout<<"after repeatstmt\n";
    }else if(Equals(str, TokenTypeStr[ID]))
    {
        //cout<<"before assignstmt\n";
        node = assignstmt(tokens, tokenInd);
        //cout<<"after assignstmt\n";
    }else if(Equals(str, TokenTypeStr[READ]))
    {
        //cout<<"before readstmt\n";
        node = readstmt(tokens, tokenInd);
        //cout<<"after readstmt\n";
    }else if(Equals(str, TokenTypeStr[WRITE]))
    {
        //cout<<"before writestmt\n";
        node = writestmt(tokens, tokenInd);
        //cout<<"after writestmt\n";
    }else{
        //cout<<"nothing to match we have "<<str<<endl;
        return nullptr;
    }
    return node;
}

// stmtseq -> stmt { ; stmt }
TreeNode* stmtseq(vector<ScanNode*>& tokens, tokenIndex* tokenInd)
{
    //cout<<"stmtseq\n";
    TreeNode* head = stmt(tokens, tokenInd);
    TreeNode* currTree = head;
    if(head == nullptr) return head;

    bool hasSemiColon = false;

    //match a semicolon after first stmt
    if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[SEMI_COLON]))
    {
        //skip semicolon
        tokenInd->value++;
        hasSemiColon = true;
    }

    //cases that must end stmtseq
    while
    (
      tokenInd->value < tokens.size() &&
      !Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ENDFILE]) &&
      !Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[END]) &&
      !Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[ELSE]) &&
      !Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[UNTIL])
    )
    {
        //cout<<"inside stmtseq loop we have "<<tokens[tokenInd->value]->tokenTypeStr<<endl;
        //parsing without a semicolon then there is a missing semicolon
        if(!hasSemiColon) { cout<<"missing semicolon at line "<<tokens[tokenInd->value]->lineNumber-1<<'\n'; return nullptr;}
        //start new stmt
        TreeNode* nextTree = stmt(tokens, tokenInd);
        currTree->sibling = nextTree;
        currTree = nextTree;

        //match a semicolon
        if(Equals(tokens[tokenInd->value]->tokenTypeStr, TokenTypeStr[SEMI_COLON]))
        {
            //skip semicolon
            tokenInd->value++;
            hasSemiColon = true;
        }else{
            hasSemiColon = false;
        }
    }

    if(hasSemiColon)
    {
        cout<<"ERROR: last statement should be without a semicolon at line "<<tokens[tokenInd->value]->lineNumber-1<<'\n';
        return nullptr;
    }

    return head;
}


// program -> stmtseq
TreeNode* parser(vector<ScanNode*>& tokens)
{
    tokenIndex* tokenInd = new tokenIndex;
    tokenInd->value = 0;
    TreeNode* syntaxTreeRoot = stmtseq(tokens, tokenInd);
    if(tokenInd->value < (tokens.size()-1))
    {
        cout<<"Error execution stops before all tokens end\n";
    }
    delete tokenInd;
    return syntaxTreeRoot;
}

int main()
{
    //get tokens list from the scanner
    vector<ScanNode*> scannerOutput = scanner("input.txt", "output.txt");

    //check for error tokens
    bool parse = true;
    for(int i=0;i<scannerOutput.size();i++)
    {
        if(Equals(scannerOutput[i]->tokenTypeStr, "Error"))
        {
            parse = false;
            cout<<"ERROR: "<<scannerOutput[i]->info<<" is not recognized at line "<<scannerOutput[i]->lineNumber<<'\n';
            break;
        }
    }

    if(parse)
    {
        TreeNode* root = parser(scannerOutput);
        if(root != nullptr) { PrintTree(root); }
    }

    return 0;
}


