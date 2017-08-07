#include <cctype>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <exception>

#define MOD 1000000007
/*
    Implementation of LL(1) expression parser
    Grammar:
        E ::= T+E | T-E | T
        T ::= F*T | F/T | F
        F ::= +F | -F | num | (E)
*/

std::string expr;
int eip;
char * msg[] = {
    NULL, "E -> TA", "A -> +E", "A -> -E", "A -> e",
    "T -> FB", "B -> *T", "B -> /T", "B -> e",
    "F -> +F", "F -> -F", "F -> ", "F -> (E)"
};

/* AST */
typedef struct tree_t
{
    char val;
    double ans;
    struct tree_t* left, *right, *right2;
} TREE, *PTREE;

PTREE make_node(char val)
{
    PTREE ret = new TREE;
    ret->ans = 0;
    ret->left = ret->right = NULL;
    ret->val = val;
    return ret;
}

int next_state(char nt, std::string str)
{
    if (isdigit(str[0])) {
        switch (nt) {
            case 'E': return 1;
            case 'T': return 5;
            case 'F': return 11;
        }
    } else {
        switch (str[0]) {
            case '+':
                switch (nt) {
                    case 'E': return 1;
                    case 'A': return 2;
                    case 'T': return 5;
                    case 'B': return 8;
                    case 'F': return 9;
                }
                break;
            case '-':
                switch (nt) {
                    case 'E': return 1;
                    case 'A': return 3;
                    case 'T': return 5;
                    case 'B': return 8;
                    case 'F': return 10;
                }
                break;
            case '*':
                if (nt == 'B') return 6;
                break;
            case '/':
                if (nt == 'B') return 7;
                break;
            case '(':
                switch (nt) {
                    case 'E': return 1;
                    case 'T': return 5;
                    case 'F': return 12;
                }
                break;
            case ')':
            case '$':
                switch (nt) {
                    case 'A': return 4;
                    case 'B': return 8;
                }
                break;
        }
    }
    return 0;
}

std::string next_token(int& old_eip)
{
    std::string ans = "";
    old_eip = eip;
    if (eip == expr.size())
        return "$";
    while (isspace(expr[eip])) ++eip;
    while (isdigit(expr[eip])) {
        ans += expr[eip++];
    }
    if (ans.size())
        return ans;
    ans += expr[eip++];
    return ans;
}

void eval(PTREE tree)
{
    if (tree) {
        eval(tree->left);
        eval(tree->right);
        PTREE l = tree->left, r = tree->right;
        switch (tree->val) {
            case 'F':
                switch (l->val) {
                    case '-':
                        tree->ans = -r->ans;
                        break;
                    case '+':
                        tree->ans = r->ans;
                        break;
                    case 'N':
                        tree->ans = l->ans;
                        break;
                    case '(':
                        tree->ans = r->ans;
                        //std::cout << "\n(Expr) " << r->ans << std::endl;                         break;                 }                 break;             case 'B':                 switch (l->val) {
                    case '*': tree->ans = r->ans; break;
                    case '/': tree->ans = 1.0/r->ans; break;
                    case 'e': tree->ans = 1.0; break;
                }
                break;
            case 'T':
                tree->ans = l->ans*r->ans;
                break;
            case 'A':
                switch (l->val) {
                    case '+': tree->ans = r->ans; break;
                    case '-': tree->ans = -r->ans; break;
                    case 'e': tree->ans = 0; break;
                }
                break;
            case 'E':
                tree->ans = l->ans+r->ans;
                break;
        }
    }
}

int main()
{
    std::ios_base::sync_with_stdio(false);
    int old;
    bool err;
    std::cout << "--------------------------------------------------\n";
    std::cout << "----------------Expression Parser-----------------\n";
    std::cout << "       Enter 'quit' without quotes to exit\n";
    std::cout << "--------------------------------------------------\n";
    do {
        err = false;
        std::cout << ">> ";
        std::getline(std::cin, expr);
        if (expr == "quit") break;
        eip = old = 0;
        std::stack<char> stk;
        std::stack<PTREE> tstk;
        PTREE root = make_node('E');
        tstk.push(root);
        stk.push('$');
        stk.push('E');
        while (! stk.empty()) {
            char top = stk.top();
            std::string tok = next_token(old);
            char* next = msg[next_state(top, tok)];
            if (top == tok[0]) {
                tstk.pop();
                stk.pop();
            } else if (! next) {
                if (isupper(top)) {
                    printf("Error: Expected Number or Expression at position %d\n", eip);
                } else {
                    printf("Error : Expected '%c' at position %d\n", top, eip);
                }
                err = true;
                break;
            } else if (top == 'F' && isdigit(tok[0])) {
                PTREE ttop = tstk.top();
                PTREE node = make_node('N');
                node->ans = stoi(tok);
                ttop->left = node;
                ttop->right = NULL;
                tstk.pop();
                stk.pop();
            } else {
                eip = old;
                char* ptr = next;
                stk.pop();
                for (; *ptr; ++ptr);
                for (--ptr; !isspace(*ptr); --ptr) {
                    if (*ptr ^ 'e')
                        stk.push(*ptr);
                }
                PTREE node = tstk.top();
                tstk.pop();
                if (next[5] == '(') {
                    PTREE ch1 = make_node(next[5]);
                    PTREE ch2 = make_node(next[6]);
                    PTREE ch3 = make_node(next[7]);
                    node->left = ch1;
                    node->right = ch2;
                    node->right2 = ch3;
                    tstk.push(ch3);
                    tstk.push(ch2);
                    tstk.push(ch1);
                } else {
                    PTREE ch1 = make_node(next[5]);
                    node->left = ch1;
                    if (! next[6]) {
                        node->right = NULL;
                    } else {
                        node->right = make_node(next[6]);
                        tstk.push(node->right);
                    }
                    if (next[5] ^ 'e') {
                        tstk.push(node->left);
                    }
                }
            }
        }
        /*
            Evaluate AST if no Syntax Error
        */
        if (! err) {
            eval(root);
            std::cout << root->ans << std::endl;
        }
    } while (1);
    return 0;
}
