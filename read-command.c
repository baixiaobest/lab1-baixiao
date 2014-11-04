// UCLA CS 111 Lab 1 command reading

// Copyleft 2014-2015 Baixiao Huang
// Copyleft 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

typedef struct command_node
{
    char *buff;
    size_t buff_size;
    char **tokens;
    unsigned int tokens_count;
    struct command_node *next_node;
    struct command_node *prev_node;
    struct command *tree_root;
    unsigned int ID;
};

typedef struct command_stream
{
    struct command_node *head;
    struct command_node *tail;
    struct command_node *cursor;
    unsigned int node_count;
};

typedef struct stack
{
    unsigned int stack_size;
    struct command **commands;
};

struct stack* create_stack()
{
    struct stack *new_stack = (struct stack*) malloc(sizeof(struct stack));
    new_stack->stack_size = 0;
    new_stack->commands = (struct command**)malloc(512*sizeof(struct command*));
    new_stack->commands[0] = NULL;
    return new_stack;
}

struct command* pop(struct stack* stack_ptr)
{
    if(stack_ptr == NULL || stack_ptr->stack_size==0) return NULL;
    stack_ptr->stack_size--;
    struct command *tail = stack_ptr->commands[stack_ptr->stack_size];
    stack_ptr->commands[stack_ptr->stack_size] = NULL;
    return tail;
}

void push(struct stack* stack_ptr, struct command *new_command)
{
    if(stack_ptr==NULL || new_command==NULL) return;
    stack_ptr->commands[stack_ptr->stack_size] = new_command;
    stack_ptr->stack_size++;
    stack_ptr->commands[stack_ptr->stack_size] = NULL;
}

struct command *top(struct stack* stack_ptr)
{
    if(stack_ptr->commands[0]==NULL)return NULL;
    return stack_ptr->commands[stack_ptr->stack_size-1];
}

/*function gets a line of char separated by '\n'
returns pointer to the first char of buffer
returns NULL if next line is end of file*/
char* getLine(void *get_next_byte_arg)
{
    size_t buffer_size = 2048;
    char *buffer = (char*) malloc(buffer_size*sizeof(char));
    buffer[0] = '\0';
    if(get_next_byte_arg!=EOF)
        fgets(buffer, buffer_size, get_next_byte_arg);
//    printf("%s\n", buffer);
    return buffer;
}

struct command *get_next_command(struct command_stream *stream)
{
    if(stream->cursor==NULL) return NULL;
    struct command *return_command = stream->cursor->tree_root;
    stream->cursor = stream->cursor->next_node;
    return return_command;
}

//function declaration
void create_tokens_for_node(struct command_node *node);

/*create a node for given text
 text is tokenized and stored in command node
 a storage of text in node is no longer supported
 text storage (char* buff) will be null pointer*/
struct command_node *create_node(char *command_text)
{
    struct command_node* newnode;
    newnode = (struct command_node*) malloc(sizeof(struct command_node));
    newnode->next_node = NULL;
    newnode->prev_node = NULL;
    newnode->tokens = NULL;
    newnode->tokens_count = 0;
    newnode->tree_root = NULL;
    if(command_text!=NULL && command_text[0]!='\0')
    {
        newnode->buff = command_text;
        create_tokens_for_node(newnode);
        free(newnode->buff);
    }
    newnode->buff = NULL;
    return newnode;
}

/*add new command node to command stream*/
void add_command_node(struct command_stream *stream, char* command_text)
{
    struct command_node *newnode = create_node(command_text);
    if(newnode->tokens==NULL || newnode->tokens[0]==NULL)return;  //we don't create a node with no token stored
    if (stream->head == NULL)
    {
        stream->head = newnode;
        stream->tail = newnode;
        stream->cursor = newnode;
        newnode->ID = 0;
        stream->node_count = 1;
    }else{
        stream->node_count++;
        stream->tail->next_node = newnode;
        newnode->ID = stream->node_count-1;
        newnode->prev_node = stream->tail;
        stream->tail = newnode;
    }
}

/*return 1 if s contains only '\n' '\t' and ' '
 otherwise return 0*/
int isEmptyString(char* s)
{
    char *ptr = s;
    int isEmpty = 1;
    while (ptr!=NULL && *ptr!='\0') {
        if (*ptr!=' ' && *ptr!= '\n' && *ptr!='\t') {
            isEmpty = 0;
        }
        ptr++;
    }
    return isEmpty;
}

/*get the last character of string, ignore ' ' '\n' '\t'*/
char endingChar(char* s)
{
    char *ptr = s;
    char curr_char = '\0';
    while (ptr!=NULL&&*ptr!='\0') {
        if(*ptr!=' '&&*ptr!='\n'&&*ptr!='\t')
            curr_char = *ptr;
        ptr++;
    }
    return curr_char;
}

char firstChar(char *s)
{
    char *ptr = s;
    char curr_char = '\0';
    while (ptr!=NULL&&*ptr!='\0') {
        if(*ptr!=' '&&*ptr!='\n'&&*ptr!='\t'){
            curr_char = *ptr;
            break;
        }
        ptr++;
    }
    return curr_char;
}

int isSpecialToken(char c)
{
    switch (c) {
        case ';':
        case '|':
        case '(':
        case ')':
        case '>':
        case '<':
            return 1;
            break;
            
        default:
            return 0;
            break;
    }
}

/*special tokens that can go before new line*/
int isValidSpecialToken(char c)
{
    if (isSpecialToken(c) && c!='>' && c!='<') {
        return 1;
    }
    return 0;
}

int isWord(char c)
{
    switch (c) {
        case '!': case '%':
        case '+': case ',':
        case '-': case '.':
        case '/': case ':':
        case '@': case '^':
        case '_': return 1;
        default:
            return 0;
    }
}

/*recalculate node IDs in stream*/
void recalculate_node_ID(struct command_stream *stream)
{
    int ID = 0;
    struct command_node *curr_node = stream->head;
    while (curr_node!=NULL) {
        curr_node->ID = ID;
        ID++;
        curr_node = curr_node->next_node;
    }
}

/*get the node given stream pointer and node ID*/
struct command_node *get_node_with_id(struct command_stream *stream, unsigned int id)
{
    unsigned int count = 0;
    struct command_node *ptr = stream->head;
    while (ptr!=NULL) {
        if(count == id) return ptr;
        ptr = ptr->next_node;
        count++;
    }
    return NULL;
}

/*concat tokens2 to tokens1, size of two tokens should be given*/
char **merge_tokens(char **tokens1, char **tokens2, unsigned int size1, unsigned int size2)
{
    unsigned int index = 0;
    for (; index<=size2; index++) {
        tokens1[size1+index] = tokens2[index];
    }
    free(tokens2);
    return tokens1;
}

/*merge from node with id1 to node with id2
 say id1 == 1, id2 == 5, then node with id 1, 2, 3, 4, 5 will be merged
 a new command_node will be linked with previous node of id1 and next node of id2
 this new node will be returned*/
struct command_node *merge_nodes_with_id(struct command_stream *stream, unsigned int id1, unsigned int id2)
{
    struct command_node *newnode = create_node(NULL);
    char **tokens = get_node_with_id(stream, id1)->tokens;
    unsigned int tokens_size = get_node_with_id(stream, id1)->tokens_count;
    unsigned int id_index = id1+1;
    for (; id_index<=id2; id_index++) {
        struct command_node *curr_node = get_node_with_id(stream, id_index);
        tokens = merge_tokens(tokens, curr_node->tokens, tokens_size, curr_node->tokens_count);
        tokens_size += curr_node->tokens_count;
    }
    newnode->tokens = tokens;
    newnode->tokens_count = tokens_size;
    newnode->prev_node = get_node_with_id(stream ,id1)->prev_node;
    newnode->next_node = get_node_with_id(stream, id2)->next_node;
    if(newnode->prev_node!=NULL){
        newnode->prev_node->next_node = newnode;
    }else{
        stream->head = newnode;
    }
    
    if(newnode->next_node!=NULL){
        newnode->next_node->prev_node = newnode;
    }else{
        stream->tail = newnode;
    }
    if(stream->cursor->ID>=id1 && stream->cursor->ID<=id2)
        stream->cursor = newnode;
    recalculate_node_ID(stream);
    return newnode;
}
/*merge the node and its next node
 buffer in node will be combined
 tokens array in node will be combined
 the next node will be destroyed*/
void merge_nodes(struct command_node *node)
{
    struct command_node *next = node->next_node;
    if (next==NULL) return;
    if(node->buff!=NULL&&next->buff!=NULL)  //new implementation
        strcat(node->buff, node->next_node->buff);

    if(node->tokens != NULL && next->tokens != NULL){
        unsigned int count=0;
        for (; count<=next->tokens_count; count++) {
            node->tokens[count+node->tokens_count] = next->tokens[count];
        }
        node->tokens_count += next->tokens_count;
    }
    node->next_node = next->next_node;
    //free(next);
}

/*split a node into two nodes,
 argument "token" indicates where in node should original tokens arrary splits,
 two new nodes will have the same ID*/
void split_node(struct command_node *node, char **token)
{
    char *text = NULL;
    struct command_node *newnode = create_node(text);
    newnode->buff = NULL;
    newnode->prev_node = node;
    newnode->next_node = node->next_node;
    newnode->tokens = (char**) malloc(512*sizeof(char*));
    newnode->tokens[0] = NULL;
    
    char **token_ptr = node->tokens;
    unsigned int tokens_index1 = 0;
    while (*(token_ptr)!=NULL&&tokens_index1<=node->tokens_count) {
        if(token_ptr==token) break;
        tokens_index1++;
        token_ptr++;
    }

    unsigned int tokens_index2 = 0;
    while (*(token_ptr)!=NULL) {
        newnode->tokens[tokens_index2] = *token_ptr;
        token_ptr++;
        tokens_index2++;
    }
    newnode->tokens_count = tokens_index2;
    node->tokens_count = tokens_index1;
    newnode->tokens[newnode->tokens_count] = NULL;
    node->tokens[node->tokens_count] = NULL;

    node->next_node->prev_node = newnode;
    node->next_node = newnode;
    newnode->ID = node->ID;
}

/*DEPRECATED FUNCTION
 Too dangerous if you use it*/
void merge_nodes_in_stream(struct command_stream *stream)
{
    struct command_node *curr_node = stream->head;
    while (curr_node!=NULL) {
        if(endingChar(curr_node->buff) == '|')
        {
            merge_nodes(curr_node);
        }else{curr_node = curr_node->next_node;}
    }
}

enum block_type{IF_BLOCK, WHILE_BLOCK, UNTIL_BLOCK, EMPTY};

/*if the last token of node is pipe, node will be merged with next node
 if "if" or "while" token is found, node will be merged next node 
 until a "fi" or "done" is found*/
void merge_nodes_in_stream2(struct command_stream *stream)
{
    struct command_node *curr_node = stream->head;
    while (curr_node!=NULL) {
        unsigned int i=0;
        enum block_type type = EMPTY;
        for (; i<curr_node->tokens_count; i++) {
            if(strcmp(curr_node->tokens[i],"if")==0) type=IF_BLOCK;
            if(strcmp(curr_node->tokens[i],"while")==0) type=WHILE_BLOCK;
            if(strcmp(curr_node->tokens[i], "until")==0) type=UNTIL_BLOCK;
            if (type!=EMPTY) {
                char *starting_char;
                char *ending_char;
                char *ending_char2;
                switch (type) {
                    case IF_BLOCK:
                        starting_char = "if";
                        ending_char = "fi";
                        break;
                    case WHILE_BLOCK:
                        starting_char = "while";
                        ending_char = "done";
                        ending_char2 = "until";
                        break;
                    case UNTIL_BLOCK:
                        starting_char = "until";
                        ending_char = "done";
                        ending_char2 = "while";
                        break;
                    default: break;
                }
//                printf("I've found %s in node: %d\n", starting_char,curr_node->ID);
                unsigned int if_count = 1;
                struct command_node *search_node = curr_node;
                unsigned int ids = search_node->ID;
                char **token_ptr = &(curr_node->tokens[i+1]);
                int if_found = 0;
                //search for "fi" or "done" through nodes
                while (search_node!=NULL)
                {
//                    printf("\tsearching node %d\n", search_node->ID);
                    //search for "fi" through tokens
                    while (*token_ptr!=NULL)
                    {
                        if(strcmp(*(char**)token_ptr,starting_char)==0
                           || ((type==WHILE_BLOCK||type==UNTIL_BLOCK)&&strcmp(*(char**)token_ptr, ending_char2)==0)){
                            if_count++;
                        };
                        if(strcmp(*(char**)token_ptr,ending_char)==0) {
                            if_count--;
                        }
                        if(if_count==0) {
//                            printf("%s found at node %d\n\tif_count %d\n", ending_char, search_node->ID, if_count);
                            if(*(token_ptr+1)!=NULL) {
                                //an error message goes here, nothing goes after ending char except < > and ; token
                                if (strcmp(*(char**)(token_ptr+1), "<")!=0
                                    &&strcmp(*(char**)(token_ptr+1), ">")!=0
                                    &&strcmp(*(char**)(token_ptr+1), ";")!=0 ) {
                                    error(1, 0, "nothing follows %s except < or > or ;", ending_char);
                                }
                                split_node(search_node, token_ptr+1);
                                recalculate_node_ID(stream);
                            }
                            if_found = 1;
                            break;
                        }
                        token_ptr++;
                    }
                    if(if_found == 1){
                        search_node = merge_nodes_with_id(stream, curr_node->ID, search_node->ID);//merge here
                        curr_node = search_node;
                        break;
                    }
                    if(search_node->next_node==NULL) {
                        // an error message goes here
                        error(1, 0, "corresponding %s for %s cannot be found", ending_char, starting_char);
                        break;
                    }
                    search_node = get_node_with_id(stream, ++ids);
                    token_ptr = search_node->tokens;
                }
                if(if_found==1) break;
            }
        }
        if(curr_node->tokens_count!=0&&curr_node->tokens[curr_node->tokens_count-1][0]=='|')
        {
            if(curr_node->next_node == NULL) error(1, 0, "Error: pipe to nothing");
            merge_nodes(curr_node);
        }else{curr_node = curr_node->next_node;}
    }
}

/*get the first token of the string
 and return extracted token.
 pointer to string(*s) will be set to the character next to
 last character of token in string*/
char *get_first_token(char **s)
{
    unsigned int buffer_size = 128;
    char *buffer = (char*) malloc(buffer_size*sizeof(char));
    buffer[0] = '\0';
    char *ptr = *s;
    while (ptr!=NULL&&*ptr!='\0')
    {
        if (*ptr!=' '&&*ptr!='\n'&&*ptr!='\t')
        {
            if(isSpecialToken(*ptr)==1
//               || isWord(*ptr)==1
               )
            {
                buffer[0] = *ptr; buffer[1] = '\0';
                ptr++;
                break;
            }else{
                unsigned int count = 0;
                while (*ptr!='\0')
                {
                    buffer[count] = *ptr;
                    count++;
                    ptr++;
                    if(*ptr==' '||*ptr=='\t'||*ptr=='\n' || *ptr=='#'
                       || isSpecialToken(*ptr)
//                       || (isWord(*ptr)&&*ptr!='.')
                       )
                        break;
                    if(count == buffer_size-1){
                        buffer_size *= 2;
                        buffer = (char*) realloc(buffer,buffer_size*sizeof(char));
                    }
                }
                buffer[count] = '\0';
                break;
            }
        }
        ptr++;
    }
    *s = ptr;
    return buffer;
}

/*create an array of tokens (an array of cString pointers)
 from buffer in the node*/
void create_tokens_for_node(struct command_node *node)
{
    if(node->buff==NULL) return;
    if(node->tokens!=NULL) free(node->tokens);
    node->tokens = (char**) malloc(512*sizeof(char*));
    node->tokens[0] = NULL;
    char *buffer = node->buff;
    char *token = get_first_token(&buffer);
    unsigned int count = 0;
    while (*token!='\0'&&*token!='#') {
        node->tokens[count] = token;
        token = get_first_token(&buffer);
        count++;
    }
    node->tokens_count = count;
    node->tokens[count] = NULL;
}

/*perform create_tokens_for_node for every node in the stream*/
void create_tokens_for_stream(struct command_stream *stream)
{
    struct command_node *node = stream->head;
    while (node!=NULL) {
        create_tokens_for_node(node);
        node = node->next_node;
    }
}


int isOperator(char* operator)
{
    if(strcmp(operator, ";")==0
    || strcmp(operator, "|")==0
    || strcmp(operator, "(")==0
    || strcmp(operator, ")")==0)
        return 1;
    return 0;
}

/////////////////////////////////////////////////////////
//PARSING ALGORITHM
/////////////////////////////////////////////////////////

enum precedence_action {POP, PUSH, POP_UNTIL_PARENTHESIS, NON_OPERATOR};

/*compare the precedence of three operator ';' '|' and '('
 it return proper parsing operation (enum in precedence_action) after the comparison*/
int compare_precedence(char* stack_operator, char* curr_operator)
{
    if(curr_operator==NULL || isOperator(curr_operator)==0) return NON_OPERATOR;
    if(stack_operator==NULL && strcmp(curr_operator, ")")!=0) return PUSH;
    if(strcmp(curr_operator, ")")==0) return POP_UNTIL_PARENTHESIS;
    if(strcmp(curr_operator, "(")==0 || strcmp(stack_operator, "(")==0) return PUSH;
    if (strcmp(stack_operator, "|")==0) {
        if(strcmp(curr_operator, "|")==0){
            return POP;
        }
        else if(strcmp(curr_operator, ";")==0) {
            return POP;
        }
        else{
            return NON_OPERATOR;
        }
    }
    else if(strcmp(stack_operator, ";")==0){
        if (strcmp(curr_operator, "|")==0) {
            return PUSH;
        }
        else if(strcmp(curr_operator, ";")==0){
            return POP;
        }else{
            return NON_OPERATOR;
        }
    }else{
        return NON_OPERATOR;
    }
}

/*create a simple command given an array of tokens
 it returns a pointer to newly created command structure*/
struct command *create_simple_command(char **tokens, unsigned int tokens_size)
{
    if(tokens_size==0) return NULL;
    struct command *simple_command = (struct command*) malloc(sizeof(struct command));
    simple_command->type = SIMPLE_COMMAND;
    
    simple_command->u.word = (char**) malloc(126*sizeof(char*));
    simple_command->u.word[0] = tokens[0];
    simple_command->u.word[1] = NULL;
    

    simple_command->input = NULL;
    simple_command->output = NULL;
    if(tokens_size==2){
        if(strcmp(tokens[1], "<")==0 || strcmp(tokens[1], ">")==0)error(1,0, "%s follows by nothing", tokens[1]);//an error message goes here
        simple_command->u.word[1] = tokens[1];
        simple_command->u.word[2] = NULL;
    }else if(tokens_size>=3){
        unsigned int index = 0;
        for (; index<tokens_size; index++) {
            if(strcmp(tokens[index], "<")==0){
                //an error message goes here, nothing follows <
                if(index+1>=tokens_size){
                    error(1,0, "Error: nothing follow <");
                };
                //an error message goes here
                if(strcmp(tokens[index+1], ">")==0 || strcmp(tokens[index+1], "<")==0)
                    error(1, 0, "%s cannot be treated as an input", tokens[index+1]);
                simple_command->input = tokens[index+1];
                //an > follows a word, indicating an output
                if(index+2<tokens_size && strcmp(tokens[index+2],">")==0){
                    // an error goes here, nothing follow >
                    if(index+3 >= tokens_size){
                        error(1, 0, "Error: nothing follow >");
                    };
                    //an error message goes here
                    if(strcmp(tokens[index+3], ">")==0 || strcmp(tokens[index+3], "<")==0)
                        error(1, 0, "%s cannot be treated as an output", tokens[index+3]);
                    simple_command->output = tokens[index+3];
                }else{
                    simple_command->output = NULL;
                }
                break;
            }else if(strcmp(tokens[index], ">")==0){
                //an error message goes here, nothing follows <
                if(index+1>=tokens_size){
                    error(1,0, "Error: nothing follow <");
                };
                //an error message goes here
                if(strcmp(tokens[index+1], ">")==0 || strcmp(tokens[index+1], "<")==0)
                    error(1, 0, "%s cannot be treated as an output", tokens[index+1]);
                simple_command->output = tokens[index+1];
                simple_command->input = NULL;
                break;
            }else{
                //do nothing
            }
        }
        unsigned int i=0;
        for (; i<index; i++) {
            simple_command->u.word[i] = tokens[i];
            simple_command->u.word[i+1] = NULL;
        }
    }
    return simple_command;
}

/*create corresponding command given an operator token
 it returns a pointer to created command structure*/
struct command* create_pipe_or_semi_or_subshell_command(char *token)
{
    enum command_type type;
    if(strcmp(token, ";")==0){
        type = SEQUENCE_COMMAND;
    }
    else if(strcmp(token, "|")==0){
        type = PIPE_COMMAND;
    }else if(strcmp(token, "(")==0){
        type = SUBSHELL_COMMAND;
    }else{return NULL;}
    struct command *new_command = (struct command*) malloc(sizeof(struct command));
    new_command->type = type;
    new_command->input = NULL;
    new_command->output = NULL;
    return new_command;
}

/*convert an operator | ; ( in form of command structure to a token representation*/
char *cvt_command_to_token(struct command* command)
{
    if(command==NULL) return NULL;
    switch (command->type) {
        case PIPE_COMMAND: return "|";
            break;
        case SEQUENCE_COMMAND: return ";";
            break;
        case SUBSHELL_COMMAND: return "(";
            break;
        default:
            break;
    }
    return NULL;
}

/*construct a tree recursively from a stack of command struct pointers*/
void tree_construct(struct command **tree_leave, struct stack *stack_ptr)
{
    if(stack_ptr->stack_size==0) return;
    if(top(stack_ptr)->type==SIMPLE_COMMAND){
        *tree_leave = pop(stack_ptr);
        return;
    }
    if(top(stack_ptr)->type==SEQUENCE_COMMAND
       || top(stack_ptr)->type==PIPE_COMMAND)
    {
        *tree_leave = pop(stack_ptr);
        if(stack_ptr->stack_size==0) {  //stand alone sequence or pipe command
            if((*tree_leave)->type == PIPE_COMMAND){ error(1, 0, "stand alone pipe");}
            else if((*tree_leave)->type == SEQUENCE_COMMAND){
                *tree_leave = NULL;
            }
            return;
        }
        else{
            tree_construct(&((*tree_leave)->u.command[1]), stack_ptr);
            if(stack_ptr->stack_size==0){
                if((*tree_leave)->type == PIPE_COMMAND)error(1,0,"pipe contains only one argument");
                (*tree_leave) = (*tree_leave)->u.command[1];
                return;
            }
            tree_construct(&((*tree_leave)->u.command[0]), stack_ptr);
        }
        return;
    }
    if(top(stack_ptr)->type==SUBSHELL_COMMAND){
        *tree_leave = pop(stack_ptr);
        tree_construct(&((*tree_leave)->u.command[0]), stack_ptr);
        return;
    }
    //write for if while until commands
}


/*easy commands are pipe, sequence, simple and subshell coommands
 function generate a tree structure for given tokens and return 
 the tree root, which is a pointer to command structure*/
struct command *parse_easy_command(char **tokens, unsigned int tokens_size)
{
    if(tokens==NULL || *tokens=='\0') return NULL;
    
    //error message goes here
    if(strcmp(tokens[0], ";")==0 || strcmp(tokens[0], "<")==0
    || strcmp(tokens[0], ">")==0)
        error(1, 0, "unexpected tokens %s in the begining of simple command", tokens[0]);
    
    struct command *root = NULL;
    struct stack* operatorStack = create_stack();
    struct stack* postfixStack = create_stack();
    char **command_tokens = (char**) malloc(126*sizeof(char*));
    command_tokens[0] = NULL;
    unsigned int command_tokens_size = 0;
    unsigned int i = 0;
    for (; i<tokens_size; i++)
    {
        enum precedence_action action = compare_precedence(cvt_command_to_token(top(operatorStack)), tokens[i]);
        if(action == NON_OPERATOR){

            command_tokens[command_tokens_size++] = tokens[i];
        }else if(action == PUSH){

            push(postfixStack, create_simple_command(command_tokens, command_tokens_size));
            command_tokens[0] = NULL;
            command_tokens_size = 0;

            push(operatorStack, create_pipe_or_semi_or_subshell_command(tokens[i]));
        }else if(action == POP){

            push(postfixStack, create_simple_command(command_tokens, command_tokens_size));
            command_tokens[0] = NULL;
            command_tokens_size = 0;

            push(postfixStack, pop(operatorStack));
            i--; //have to check operatorStack again before moving on
        }else if(action == POP_UNTIL_PARENTHESIS){
            //an error goes here, may be dangerous
//            if(command_tokens_size == 0)
//                error(1, 0, "nothing is between ( and )");
            push(postfixStack, create_simple_command(command_tokens, command_tokens_size));
            command_tokens[0] = NULL;
            command_tokens_size = 0;

            struct command *ptr;
            do{
                //an error message goes here
                if(operatorStack->stack_size==0)error(1, 0, "cannot find matching '('");
                ptr = pop(operatorStack);
                push(postfixStack, ptr);
            }while(ptr->type != SUBSHELL_COMMAND);
        }else{
            //not gonna happen
        }
    }
    if(command_tokens_size!=0){
//        printf("push operator %s to stack\n", command_tokens[0]);  //debugging
        push(postfixStack, create_simple_command(command_tokens, command_tokens_size));
    }

    //push all remainint operator from operator stack
    unsigned int size = operatorStack->stack_size;
    while(size>0) {
        //an error message goes here
        if(top(operatorStack)->type == SUBSHELL_COMMAND)
            error(1, 0, "( does not match all ) token");
        push(postfixStack, pop(operatorStack));
        size = operatorStack->stack_size;
    }
    
    tree_construct(&root, postfixStack);
    
    return root;
}

/*function prototype here*/
struct command *parse_tokens(char **tokens, unsigned int tokens_size);

/*parse the if command recursively
 it returns a pointer to if command structure
 it recursively calls parse_tokens function*/
struct command *parse_if_command(char **tokens, unsigned int tokens_size)
{
    if(tokens==NULL || *tokens==NULL) return NULL;
    struct command *if_command = (struct command*) malloc(sizeof(struct command));
    if_command->type = IF_COMMAND;
    
    if_command->output = NULL;
    if_command->input = NULL;
    
    struct command *condition_command = NULL;
    char **condition_tokens = &tokens[1];
    unsigned int condition_index = 0;
    unsigned int if_count = 1;
    unsigned int valid_else = 0;
    while(condition_index<tokens_size-1){
        if(strcmp(condition_tokens[condition_index], "if")==0){
            if_count++;
            valid_else++;
        }
        else if(strcmp(condition_tokens[condition_index], "then")==0){
            if_count--;
        }
        // an error message goes here if else appears before then
        else if(strcmp(condition_tokens[condition_index], "else")==0
                && valid_else==0){
            error(1, 0, "else does not go before then");
        }
        if(if_count==0){
            break;
        }
        condition_index++;
    }
    condition_command = parse_tokens(condition_tokens, condition_index);
    
    struct command *then_list_command = NULL;
    char **then_list_tokens = &condition_tokens[condition_index+1];
    unsigned int then_list_index = 0;
    if_count = 1;
    while (then_list_tokens[then_list_index]!=tokens[tokens_size]) {
        if(strcmp(then_list_tokens[then_list_index], "if")==0){
            if_count++;
        }
        else if(strcmp(then_list_tokens[then_list_index], "else")==0){
            if_count--;
        }else if(strcmp(then_list_tokens[then_list_index], "fi")==0){
            break;
        }
        if(if_count==0){
            break;
        }
        then_list_index++;
    }
    then_list_command = parse_tokens(then_list_tokens, then_list_index);
    
    struct command *else_list_command = NULL;
    if(strcmp(then_list_tokens[then_list_index], "else")==0){
        char **else_list_tokens = &then_list_tokens[then_list_index+1];
        unsigned int else_list_index = 0;
        if_count = 1;
        while(else_list_tokens[else_list_index]!=tokens[tokens_size]){
            if(strcmp(else_list_tokens[else_list_index], "if")==0){
                if_count++;
            }
            else if(strcmp(else_list_tokens[else_list_index], "fi")==0){
                if_count--;
            }
            if(if_count==0)
                break;
            else_list_index++;
        }
    else_list_command = parse_tokens(else_list_tokens, else_list_index);
        
    }
    
    if_command->u.command[0] = condition_command;
    if_command->u.command[1] = then_list_command;
    if_command->u.command[2] = else_list_command;
    
    return if_command; //not final
}

struct command *parse_while_until_command(char **tokens, unsigned int tokens_size)
{
    if(tokens==NULL || *tokens==NULL) return NULL;
    if(tokens_size == 0) return NULL;
    enum command_type type;
    if(strcmp(tokens[0], "while")==0){
        type = WHILE_COMMAND;
    }else{
        type = UNTIL_COMMAND;
    }
    struct command *loop_command = (struct command*) malloc(sizeof(struct command));
    loop_command->type = type;
    loop_command->input = NULL;
    loop_command->output = NULL;
    
    struct command *condition_command = NULL;
    char **condition_tokens = &tokens[1];
    unsigned int condition_index = 0;
    unsigned int keyword_count = 1;
    while(condition_index<tokens_size-1){
        
        if(strcmp(condition_tokens[condition_index], "while")==0
        || strcmp(condition_tokens[condition_index], "until")==0){
            keyword_count++;
        }
        else if(strcmp(condition_tokens[condition_index], "do")==0){
            keyword_count--;
        }
        if(keyword_count==0){
            break;
        }
        
        condition_index++;
    }

    condition_command = parse_tokens(condition_tokens, condition_index);
    
    struct command *loop_list_command = NULL;
    char **loop_list_tokens = &condition_tokens[condition_index+1];
    unsigned int loop_list_tokens_index = 0;
    keyword_count = 1;
    while (loop_list_tokens[loop_list_tokens_index]!=tokens[tokens_size])
    {
        if(strcmp(loop_list_tokens[loop_list_tokens_index], "while")==0
           || strcmp(loop_list_tokens[loop_list_tokens_index], "until")==0){
            keyword_count++;
        }
        else if(strcmp(loop_list_tokens[loop_list_tokens_index], "done")==0){
            keyword_count--;
        }
        if(keyword_count==0){
            break;
        }
        loop_list_tokens_index++;
    }

    loop_list_command = parse_tokens(loop_list_tokens, loop_list_tokens_index);
    
    //if there are other tokens after done
    if(loop_list_tokens[loop_list_tokens_index]!=tokens[tokens_size-1])
    {
        if(strcmp(loop_list_tokens[loop_list_tokens_index+1], ">")!=0){
            struct command *extra_command = NULL;
            char **extra_tokens = &(loop_list_tokens[loop_list_tokens_index+1]);
            unsigned int extra_tokens_index = 0;
            while (extra_tokens[extra_tokens_index]!=tokens[tokens_size]) {
                extra_tokens_index++;
            }
            extra_command = parse_tokens(extra_tokens, extra_tokens_index);
            struct command *joint_command = create_pipe_or_semi_or_subshell_command(";");
            joint_command->u.command[0] = loop_command;
            joint_command->u.command[1] = extra_command;
        
            loop_command->u.command[0] = condition_command;
            loop_command->u.command[1] = loop_list_command;
        
            return joint_command;
        }else{
            loop_command->output = loop_list_tokens[loop_list_tokens_index+2];
        }
    }
    
    loop_command->u.command[0] = condition_command;
    loop_command->u.command[1] = loop_list_command;
    
    return loop_command;
    
}

/*parse four kinds of command stored as a array of tokens
 IF_COMMAND, WHILE_COMMAND, UNTIL_COMMAND and other commands*/
struct command *parse_tokens(char **tokens, unsigned int tokens_size)
{
    if(tokens==NULL || *tokens=='\0') return NULL;
    struct command* tree_root = NULL;
    if(strcmp(tokens[0], "if")==0)
    {
        tree_root = parse_if_command(tokens, tokens_size);//parse "if" algorithm
    }
    else if(strcmp(tokens[0], "while")==0 || strcmp(tokens[0], "until")==0)
    {
        tree_root = parse_while_until_command(tokens, tokens_size);
    }
    else if(strcmp(tokens[0], "do")==0 || strcmp(tokens[0], "done")==0
        || strcmp(tokens[0], "fi")==0){
        // an error message goes here
        error(1, 0, "unexpected token %s", tokens[0]);
    }
    else
    {
        tree_root = parse_easy_command(tokens, tokens_size);//parse sequential pipe and subshell command algorithm
    }
    return tree_root;
}

/*parse every node in the stream*/
void parse_nodes_in_stream(struct command_stream* stream)
{
    struct command_node *node = stream->head;
    while (node!=NULL) {
//        printf("parsing node %d\n", node->ID);
        struct command* root = parse_tokens(node->tokens, node->tokens_count);
        node->tree_root = root;
        node = node->next_node;
    }
}


/////////////////////////////////////////////////////////////////
//DEBUG FUNCTION
/////////////////////////////////////////////////////////////////

void print_stack_contents(struct stack* stack_ptr)
{
//    if(stack_ptr->stack_size==0){printf("nothing is here\n"); return;}
    unsigned int i=0;
    for (; i<stack_ptr->stack_size; i++) {
        unsigned k = 0;
        switch (stack_ptr->commands[i]->type) {
            case PIPE_COMMAND:
                printf("| =>");
                break;
            case SEQUENCE_COMMAND:
                printf("; =>");
                break;
            case SUBSHELL_COMMAND:
                printf("SUBSHELL COMMAND=>");
                break;
            case SIMPLE_COMMAND:
                while(stack_ptr->commands[i]->u.word[k]!=NULL){
                    printf("%s ", stack_ptr->commands[i]->u.word[k++]);
                }
                if(stack_ptr->commands[i]->input != NULL)printf(" < %s ", stack_ptr->commands[i]->input);
                if(stack_ptr->commands[i]->output != NULL)printf(" > %s ", stack_ptr->commands[i]->output);
                printf("=>");
                break;
                
            default:
                break;
        }
    }
    printf("\n");
}

void indent_print(unsigned int indent)
{
    unsigned int i=0;
    for (; i<indent; i++) {
        printf("\t");
    }
}

/*for debugging purposes, print tree of command*/
void print_tree(struct command *command_tree_node, int indent)
{
    if(command_tree_node==NULL) return;
    indent_print(indent);
    char **ptr = command_tree_node->u.word;
    switch (command_tree_node->type) {
        case PIPE_COMMAND:
            printf("PIPE COMMAND\n");
            print_tree(command_tree_node->u.command[0], indent+1);
            print_tree(command_tree_node->u.command[1], indent+1);
            break;
        case SEQUENCE_COMMAND:
            printf("SEQUENCE COMMAND\n");
            print_tree(command_tree_node->u.command[0], indent+1);
            print_tree(command_tree_node->u.command[1], indent+1);
            break;
        case SUBSHELL_COMMAND:
            printf("SUBSHELL COMMAND\n");
            print_tree(command_tree_node->u.command[0], indent+1);
            break;
        case SIMPLE_COMMAND:
            printf("SIMPLE COMMAND ");
            while (ptr!=NULL && *ptr!=NULL) {
                printf("%s ", *ptr);
                ptr++;
            }
            printf("\n");
            break;
        case IF_COMMAND:
            printf("if\n");
            print_tree(command_tree_node->u.command[0], indent+1);
            indent_print(indent);
            printf("then\n");
            print_tree(command_tree_node->u.command[1], indent+1);
            indent_print(indent);
            printf("else\n");
            print_tree(command_tree_node->u.command[2], indent+1);
            break;
        case WHILE_COMMAND:
            printf("while\n");
            print_tree(command_tree_node->u.command[0], indent+1);
            indent_print(indent);
            printf("do\n");
            print_tree(command_tree_node->u.command[1], indent+1);
            indent_print(indent);
            printf("done\n");
            break;
        case UNTIL_COMMAND:
            printf("until\n");
            print_tree(command_tree_node->u.command[0], indent+1);
            indent_print(indent);
            printf("do\n");
            print_tree(command_tree_node->u.command[1], indent+1);
            indent_print(indent);
            printf("done\n");
            break;
        default:
            break;
    }
}

/*for debugging purposes
 prints out contents of command nodes*/
void print_command_stream(struct command_stream* stream)
{
    struct command_node *curr_node = stream->head;
    unsigned int count = 0;
    while (curr_node!=NULL) {
        printf("node %d: ", curr_node->ID+1);
        
        int token_count = 0;
        for(;curr_node->tokens[token_count]!=NULL; token_count++)
        {
            printf("%s ", curr_node->tokens[token_count]);
        }
        printf("\n");
        
        print_tree(curr_node->tree_root, 0);
//        if(curr_node->tree_root!=NULL) printf("I have tree!\n");
        
        curr_node = curr_node->next_node;
        count++;
    }
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (1, 0, "command reading not yet implemented");
    char* c;
    struct command_stream *stream = (struct command_stream*) malloc(sizeof(struct command_stream));
    
    c = getLine(get_next_byte_argument);
    while(c[0]!='\0')
    {
        if (isEmptyString(c)==0){
            char endingC = endingChar(c);
            switch (endingC) {
                case '<':
                case '>':
                case '`':
                    error(1, 0, "Error: unexpected %c before newline", endingC);
                    break;
            }
            
            add_command_node(stream, c);
        }
        c = getLine(get_next_byte_argument);
    }
    merge_nodes_in_stream2(stream);
    parse_nodes_in_stream(stream);
    //print_command_stream(stream);
    
    return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
    //error(1, 0, "not yet implemented");
    struct command* next_command = get_next_command(s);
    return next_command;
}
