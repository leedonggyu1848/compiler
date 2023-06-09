#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define Max_symbols			200  // Number of terminals and nonterminals must be smaller than this number.
#define Max_size_rule_table		100  //  actual number of rules must be smaller than this number.
#define Max_string_of_RHS	20	// 룰 우측편의 최대 길이
#define MaxNumberOfStates	200  // 최대 가능한 스테이트 수.
/////

typedef struct tkt { // 하나의 토큰을 나타내는 구조체.
	int kind;  // kind : 토큰의 종류 즉 토큰번호를 말함.
	char sub_kind[3];  // kind가 relop이면 "ge","gt" 등 종류를, num 이면 정수/실수("in"/"do")를 구분.
	int sub_data; // ID 토큰의 심볼테이블엔트리 번호, 또는 num 토큰에서 정수 값을 저장함.
	double rnum;  // num 토큰인 경우 실수라면  실수값을 여기에 저장.
	char str[30]; // 토큰을 구성하는 표층 스트링을 여기에 저장.(주의: 토큰의 이름을 저장하는 것이 아님.)
} tokentype;

typedef struct ssym {
	 int kind ; // 단말/비단말 여부. (0:단말기호, 1:비단말기호)
	 int no ; // 단말, 비단말 각 경우에서 심볼의 번호.
	 char str[30]; // 단말기호의 경우 get_next_token 함수가 lexan 한테 받은 token의 표층 스트링을 여기에 저장함.
} sym ;  // 문법심볼 정의

// 룰 하나를 나타내는 구조체.
typedef struct struc_rule {  // rule 한 개의 정의.
    sym leftside ;
    sym rightside [Max_string_of_RHS] ;
    int  rleng ;  // 룰의 RHS 의 심볼의 수.  0 is given if righthand side is epsilon.
} ty_rule ;

typedef struct {
	int r, X, i, Yi;
} type_recompute;	// first 계산에서 이용하는 재계산점

//  Item list에서 사용할 구조체
typedef struct struc_itemnode* ty_ptr_item_node ;
typedef struct struc_itemnode {
	int		RuleNum ;
	int		DotNum ;
	ty_ptr_item_node	link ;
} ty_item_node ;

// state node : Goto graph에서 사용할 state node 구조체
typedef struct struc_state_node* ty_ptr_state_node;
typedef struct struc_state_node {
	int					id;				// 자신의 ID(번호)
	int					item_cnt;		// 보유하고 있는 Item 들의 수
	ty_ptr_item_node	first_item;		// 보유하고 있는 Item_list 의 시작주소
	ty_ptr_state_node	next;		// 다음 state로의 link
} ty_state_node;

typedef struct struc_arc_node* ty_ptr_arc_node;
typedef struct struc_arc_node {
	int		from_state;
	int		to_state;
	sym		symbol;
	ty_ptr_arc_node	link;
} ty_arc_node;

// types for goto graph
typedef struct struc_goto_graph* ty_ptr_goto_graph;
typedef struct struc_goto_graph {
	ty_ptr_arc_node		first_arc;				// 아크 리스트의 첫 노드를 가리킴.
	ty_ptr_state_node	first_state;	// 스테이트 노드 리스트의 첫 노드를 가리킴.
} ty_goto_graph;

typedef struct cell_action_tbl {
	char Kind ; // s, r, a, e 중 한 글자 /
	int num ;	// s 이면 스테이트 번호, r 이면 룰 번호를 나타냄.
} ty_cell_action_table ;

// 파스트리 노드 정의. 노드는 문법심볼을 가짐. 그런데, 이 노드는 LR stack 에도 넣음.
// 그런데 LR stack 에는 state 번호도 들어가야 함. 그래서 node 가 문법심볼과 state 를 넣는 2 가지 용도로 사용함.
typedef struct struc_tree_node* ty_ptr_tree_node;
typedef struct struc_tree_node { //
	sym	nodesym;	// 단말 기호나 비단말 기호를 저장.
	int	child_cnt;	// number of children
	ty_ptr_tree_node	children[10]; // 자식 노드들에 대한 포인터.
	int rule_no_used; // 비단말기호 노드인 경우 이 노드를 만드는데 사용된 룰번호.
}  ty_tree_node;

// LR 파서가 사용하는 stack 의 한 원소.
// state 정보를 가진다면: state >= 0 이상(&& symbol_node 는 NULL); 문법심볼 정보라면 state==-1 .
typedef struct struc_stk_element {
	int state;	// state 번호를 나타냄.
	ty_ptr_tree_node symbol_node;	// 문법심볼을 가지는 트리노드에 대한 포인터.
} ty_stk_element;

/////////// Function prototypes
ty_ptr_state_node	add_a_state_node_if_it_does_not_exist(ty_ptr_state_node State_Node_List_Header, ty_ptr_item_node To_list);
void	add_arc_if_it_does_not_exist(ty_ptr_arc_node* Arc_List_Header, int from_num, int to_num, sym Symbol);
ty_ptr_item_node closure(ty_ptr_item_node IS);
int		delete_and_free_items_in_item_list(ty_ptr_item_node item_list);
int		first(sym X);
void	first_all();
void	first_of_string(sym alpha[], int length, int first_result[]);
int		find_arc_in_arc_list(ty_ptr_arc_node arc_list, int from_id, sym symbal);
void	fitemListPrint(ty_ptr_item_node IS, FILE* fpw);
int		follow_nonterminal(int idx_NT);
ty_ptr_item_node	getLastItem(ty_ptr_item_node it_list);
// sym		get_next_token(FILE* fps);
ty_ptr_item_node	goto_function(ty_ptr_item_node IS, sym sym_val);
int is_item_in_itemlist(ty_ptr_item_node item_list, ty_ptr_item_node item);
int is_nonterminal_in_stoplist(int Y);
int iswhitespace(char c);
void	itemListPrint(ty_ptr_item_node IS);
int	length_of_item_list(ty_ptr_item_node IS);
int lookup_keyword_tbl(char* str);
int lookup_symtbl(char* str);
int		lookUp_nonterminal(char* str);
int		lookUp_terminal(char* str);
ty_ptr_arc_node	makeArcNode(void);
void	make_goto_graph(ty_ptr_item_node IS_0);
ty_ptr_state_node	makeStateNode(void);
int nonterminal_is_in_stack(int Y);
void	print_Action_Table(void);
void	printGotoGraph(ty_ptr_goto_graph gsp);
void	read_grammar(char[]);
int same_test_of_two_item_lists(ty_ptr_item_node list1, ty_ptr_item_node list2);

///////////   GLOBAL VARIABLES  /////////////////////////////////////////////////////////////////////////////////

int Max_terminal; //  $ 를 포함한 이 문법의 총 단말기호의 수.
int Max_nonterminal; // 이 문법이 가진 총 비단말 기호 수 (주: augmented starting nonterminal 포함).
sym Nonterminals_list [Max_symbols];	// 모든 비단말기호 들의 이름을 가짐
sym Terminals_list [Max_symbols];		// 모든 단말기호 들의 이름을 가짐

int Max_rules; // 이 문법 화일에 정의된 규칙의 총 수
ty_rule rules[Max_size_rule_table]; // 규칙 저장 테이블

int First_table[Max_symbols][Max_symbols]; // actual region:  Max_nonterminal  X (MaxTerminals+2) : 2 is for epsilbon and done_first flag
int done_first[Max_symbols];	// 비단말기호의 first 계산 완료 플래그
int Follow_table[Max_symbols][Max_symbols]; // actual region:  Max_nonterminal  X (MaxTerminals+1) : 1 is for done_first flag
int done_follow[Max_symbols];  // 비단말기호의 follow 계산 완료 플래그
type_recompute recompute_list[500];    // 재계산점들의 리스트 (first 계산에서)
int num_recompute = 0; // recompute_list에 저장된 재계산점의 총 수.
type_recompute a_recompute;

//first 계산에서 이용된는 call history stack. -1 은 더미 값. 초기화 안해도 상관없다.
int ch_stack[100] = { -1, };
int top_ch_stack = -1;		// ch_stack 의 top.

// goto_graph  구조체에 대한 포인터 (이 구조체는 arc list와 state list 의 헤더들을 가짐)
ty_ptr_goto_graph	Header_goto_graph = NULL;
int total_num_states = 0; // the actual number of states of go-to graph
int total_num_arcs = 0; // the actual number of arcs

FILE *fps; // file pointer to a source program file.

int iswhitespace(char c){
	if (c == ' ' || c == '\n' || c == '\t')
		return 1;
	else
		return 0;
}

// // get the next terminal from the source program file by calling the lexical analyzer lexan.
// sym get_next_token(FILE * fps)  {
// 	sym a_terminal_sym;
// 	tokentype a_tok; // the token produced by the  lexical analyer we developed before.
// 	a_tok = lexan_mini_grammar (fps);
// 	a_terminal_sym.kind = 0;  // this is a terminal symbol

// 	a_terminal_sym.no = a_tok.kind;	// 단말기호 고유번호
// 	// small grammar 의 경우 source program 이 단말기호의 이름을 그대로 이용함.
// 	strcpy(a_terminal_sym.str, a_tok.str);

// 	return a_terminal_sym;
// } // get_next_token


// state node 하나를 할당 받아서 이에 대한 포인터를 반환함.
ty_ptr_state_node	makeStateNode(void){
	ty_ptr_state_node cur;

	cur = (ty_ptr_state_node)malloc(sizeof(ty_state_node));

	cur->id         = -1;
	cur->item_cnt   = 0;
	cur->first_item = NULL;
	cur->next  = NULL;

	return cur;
} // makeStateNode ( )

// arc node 하나를 할당 받아서 이에 대한 포인터를 반환함.
ty_ptr_arc_node	makeArcNode(void){
	ty_ptr_arc_node cur;

	cur = (ty_ptr_arc_node)malloc(sizeof(ty_arc_node));

	cur->from_state = -1;
	cur->to_state  = -1;
	cur->symbol.kind = -1;
	cur->symbol.no   = -1;
	cur->link = NULL;

	return cur;
} // makeArcNode ( )

// Find the number of items in item list IS
int	length_of_item_list(ty_ptr_item_node IS){
	int					cnt    = 0;
	ty_ptr_item_node	cursor = IS;

	while(cursor){
		cnt++;
		cursor = cursor->link;
	} // while : cursor

	return cnt;
} // length_of_item_list ( )

// 두 아이템 리스트가 같은지 테스트한다 (가정: 아이템들이 정렬되지 않았다고 봄).
// 동일하면 1, 다르면 0 을 반환.
int same_test_of_two_item_lists ( ty_ptr_item_node list1,  ty_ptr_item_node list2 ) {
	int l1, l2 , p1_exists_in_list2;
	ty_ptr_item_node p1 = list1;
	ty_ptr_item_node p2;

	l1 = length_of_item_list(list1); l2 = length_of_item_list(list2);	// 리스트 내의 아이템 수를 알아 냄.
	if (l1 != l2)
		return 0 ;	// 길이가 다르면 동일할 수 없음.

	while( p1 ){
		// check if p1 exists in list2.
		p2 = list2 ; // take one item in list2.
		p1_exists_in_list2 = 0 ; // initially, assume that it does not exist.

		while(p2) { //try to find p1 in list2 by scanning list2 using p2.
			if(p1->RuleNum == p2->RuleNum && p1->DotNum == p2->DotNum){
				p1_exists_in_list2 = 1 ; // p1 is found in list2.
				break;
			}

			p2 = p2->link;
		} // while.

		if ( ! p1_exists_in_list2 )
			return 0 ; // the two lists are different since an item in list1 does not exist in list2.
		else
			p1 = p1->link;	// 다음 아이템으로 간다.
	} // while.

	return 1;	// 모든 test 를 통과함.  1 을 반환.
} //same_test_of_two_item_lists

//  To_item_list 를  이미 존재하는 모든 state 의 item list와 비교한다.
//  같은 것이 발견되면, 이 발견된 state 의 노드 포인터를 반환;
//  발견이 안되면 To_item_list 를 가지는 새로운 state 를 만들어 등록(state node 를 새로 만들어 맨 뒤에 붙임)하고
//  이 새 state node 에 대한 포인터를 반환.
ty_ptr_state_node	add_a_state_node_if_it_does_not_exist
		(ty_ptr_state_node State_Node_List_Header, ty_ptr_item_node To_item_list){
	int Number_Of_Items	= 0;	// 새로운 item list의 item 개수를 저장하는 변수
	ty_ptr_state_node		curr_state			= State_Node_List_Header;
	ty_ptr_state_node		prev_state		= NULL;
	ty_ptr_state_node		new_state_node	= NULL;

	Number_Of_Items = length_of_item_list(To_item_list);

	// To_item_list 가 모든 state 의 item list 와 달라야 새로운 state 로 추가한다.
	while(curr_state){
		// item 개수가 서로 같은지 확인함.
		if(curr_state->item_cnt != Number_Of_Items){  // cursor 가 가리키는 스테이드의 아이템 리스트의 아이템 수와 다르므로 다음 스테이트로 감.
			prev_state = curr_state;
			curr_state		= curr_state->next;
			continue;
		} // if : state node의 item list에 보유하고 있는 item의 개수가 일치 하지 않으면 확인하지 않아도 괜찮습니다.

		int is_same = same_test_of_two_item_lists ( curr_state->first_item, To_item_list) ; // 동일하면 1, 아니면 0 을 받음.
		if ( is_same ) { // curr_state 의 아이템리스트와 To_item_list 가 동일한 상황임.
			delete_and_free_items_in_item_list (To_item_list) ; // free all items in To_list.
			return curr_state ; // 동일한 것으로 판명된 현 스테이트 노드의 포인터를 반환해 줌.
		}

		// 두 아이템 리스트가 다른 것으로 판명되었으므로 다음 스테이트 노드로 커서를 옮김.
		prev_state = curr_state;
		curr_state = curr_state->next;
	} // while

	// state node list 전체를 비교해도 같은 아이템리스트를 가진 스테이드 노드가 발견되지 않았음.
	// 따라서 새로운 스테이드 노드를 만들어 To_item_list 를 붙이고 이 노드를 스테이트 노드 리스트의 맨 뒤에 추가함.
	new_state_node = makeStateNode();	// 새로운 state node 를 할당.

	// state  node list 맨 끝에 새로운 state node 를 연결합니다.
	new_state_node->item_cnt   = Number_Of_Items;
	new_state_node->first_item = To_item_list;
	new_state_node->id	= prev_state-> id + 1;	// state 번호를 부여.
	new_state_node->next = NULL;
	prev_state->next	= new_state_node;	// 매단다.

	// 전역변수 total_num_states 의 크기를 증가시켜줍니다.
	total_num_states++;

	return new_state_node;	// To_item_list 를 가진 state node 를 반환.
} // add_a_state_node_if_it_does_not_exist ( )

// add an arc (from_num, to_num, Symbol) only if it is not in the arc list.
void	add_arc_if_it_does_not_exist(ty_ptr_arc_node *Arc_List_Header, int from_num, int to_num, sym Symbol ){
	ty_ptr_arc_node	Arc_curr, Arc_last = NULL ;
	ty_ptr_arc_node	Arc_new = NULL;
	int same_arc_exists = 0;

	Arc_new = makeArcNode();

	Arc_new->from_state = from_num;
	Arc_new->to_state  = to_num;
	Arc_new->symbol = Symbol;
	Arc_new->link = NULL;

	if (*Arc_List_Header == NULL) {	// 아크 리스트에 비어 있다.
		*Arc_List_Header = Arc_new; // 첫 아크로 매 달아 준다.
		total_num_arcs++;
		return;
	}

	Arc_curr = *Arc_List_Header ;
	while ( Arc_curr ) 		{
		if ( Arc_curr->from_state == from_num && Arc_curr->to_state == to_num
			&& Arc_curr->symbol.kind == Symbol.kind && Arc_curr->symbol.no == Symbol.no)	{
			same_arc_exists = 1;	// 동일한 아크가 이미 존재함.
			break;
		}
		else {
			Arc_last = Arc_curr ;
			Arc_curr = Arc_curr->link ;
		}
	} // while

	if (!same_arc_exists) {
		Arc_last->link = Arc_new;	// 새로운 아크를 매달아 준다.
		total_num_arcs++;
	}
	else
		free(Arc_new);
} // add_arc_if_it_does_not_exist ( )

// Make a goto graph.
// 입력으로 넣을 0 번 스테이트를 받는다.
// 작업 결과는 state node list 와 arc node list 이다.
// 함수의 맨 마지막 부분에서 이들에 대한 포인터를 전역변수 Header_goto_graph 에 넣는다.
void	make_goto_graph(ty_ptr_item_node IS_0){
	// 지역변수 선언
	int	i_0, i_1, i_max;
	sym sym_temp;
	ty_ptr_state_node		State_Node_List_Header	 = NULL;	// 스테이트 리스트의 헤더
	ty_ptr_arc_node			Arc_List_Header = NULL;	// 아크 리스트의 헤더
	ty_ptr_state_node		next_state	= NULL;		// 현재 작업중인 스테이드 노드에 대한 포인터.
	ty_ptr_state_node		to_state	= NULL;	    // goto 에 따라 만들 새 state 노드에 대한 포인터.
	ty_ptr_item_node		To_item_list = NULL;	// goto 에 따라 알아 낸 새로운 item list.

	// 스테이트 0 를 빈 state node list 의 첫 노드에 넣도록 함.
	State_Node_List_Header = makeStateNode();
	State_Node_List_Header->id = 0 ;
	State_Node_List_Header->item_cnt = length_of_item_list(IS_0) ;
	State_Node_List_Header->first_item = IS_0; // item list 의 첫 노드를 가리키게 함.
	State_Node_List_Header->next = NULL ;
	total_num_states = 1 ;
	next_state = State_Node_List_Header ;

	while(next_state){
		// 단말기호, 비단말기호
		for(i_0 = 0 ; i_0 < 2; i_0++) { // i_0 = 0 이면 단말기호에 대한 처리이고, 1 이면 비단말기호에 대한 처리임.
			// i_max 는 각 경우(단말/비단말) 가 가진 총 기호의 수를 가지게 함.
			i_max = i_0 ? Max_nonterminal : Max_terminal-1; // Max_terminal-1 의 이유는 $ 단말기호를 무시하기 위함.
			for(i_1 = 0; i_1 < i_max; i_1++){ // i_1 은 단말/비단말 각 경우에서 기호의 번호를 나타냄.
				// 문법 심볼 하나를 만듬.
				sym_temp.kind = i_0;	// 단말, 비단말 여부
				sym_temp.no   = i_1;	// symbol 의 고유번호
				// 현재 스테이트에서 문법심볼 sym_temp 에 의해 goto 할 아이템 리스트를 알아 옴.
				To_item_list = goto_function(next_state->first_item, sym_temp);  // goto 함수로 goto 결과를 알아 옴.

				if(To_item_list){ // To_item_list 가 empty 가 아니면, 이를 새 state 로 추가.
					//  To_item_list 를 새로운 state 로 등록한다 (단, 동일한 것이 존재하지 않는 경우에만).
					//  다음 함수는 이 작업을 하고, To_item_list 를 가지는 state node 의 포인터를 반환한다.
					to_state = add_a_state_node_if_it_does_not_exist(State_Node_List_Header, To_item_list);

					// [next_state, to_state, sym_temp] 아크가 이미 존재하지 않으면 새로운 아크로 추가함.
					add_arc_if_it_does_not_exist (&Arc_List_Header, next_state->id, to_state->id, sym_temp);
				} // if : To_item_list
			} // for : i_1 : 모든 기호에 대하여 goto 를 수행
		} // for : i_0 : 단말기호 → 비단말기호
		next_state = next_state->next;
	} // while : next_state

	// state node list 와 arc list 가 만들어 졌다. 이들을 구조체에 저장하고 이 구조체의 포인터를
	// 전역변수 Header_goto_graph 에 저장한다.
	Header_goto_graph = (ty_ptr_goto_graph)malloc(sizeof(ty_goto_graph));
	Header_goto_graph->first_state = State_Node_List_Header;
	Header_goto_graph->first_arc = Arc_List_Header;
} // : make_goto_graph ( )

// Delete all items in the item_list.
int	delete_and_free_items_in_item_list(ty_ptr_item_node item_list){
	ty_ptr_item_node item_next   = NULL;
	ty_ptr_item_node curr_item = item_list->link;

	while(curr_item){
		item_next = curr_item->link;
		free(curr_item);
		curr_item = item_next;
	} // while : curr_item
	free(item_list);
	return 0;
} // delete_and_free_items_in_item_list ( )

// goto-graph 를 파일에 출력한다. 파일명: goto_graph.txt
void	printGotoGraph(ty_ptr_goto_graph gsp){
	ty_ptr_state_node		State_Node_List_Header = gsp->first_state;
	ty_ptr_arc_node		item_list  = gsp->first_arc;
	char str[10];
	FILE *fpw;
	fpw = fopen("goto_graph.txt", "w");

	while(State_Node_List_Header){
		fprintf(fpw, "ID[%2d] (%3d) : ", State_Node_List_Header->id, State_Node_List_Header->item_cnt);
		fitemListPrint(State_Node_List_Header->first_item, fpw);
		State_Node_List_Header = State_Node_List_Header->next;
	} // while : State_Node_List_Header
	printf("\nTotal number of states = %d\n", total_num_states);
	fprintf(fpw, "Total number of states = %d\n", total_num_states);

	fprintf(fpw, "\nGoto arcs:\nFrom   To  Symbol\n");
	while(item_list){
		fprintf(fpw, "%4d %4d    ", item_list->from_state, item_list->to_state);
		if (item_list->symbol.kind == 0)
			strcpy(str, Terminals_list[item_list->symbol.no].str );
		else
			strcpy(str, Nonterminals_list[item_list->symbol.no].str );
		fprintf(fpw, "%s \n", str);

		item_list = item_list->link;
	} // while : item_list
	printf("Total number of arcs = %d\n", total_num_arcs);
	fprintf(fpw, "Total number of arcs = %d\n", total_num_arcs);
	fclose(fpw);
} // printGotoGraph

// 입력:  arc_list,  from state 번호,  grammar symbol
// 아크 리스트에서 주어진 from_state, grammar symbol 의 아크를 찾아서 있으면 to_state 번호를 반환(없으면 -1).
int		find_arc_in_arc_list ( ty_ptr_arc_node arc_list, int from_id, sym asym) {
	ty_ptr_arc_node	curr = arc_list;

	while (curr) {
		if(curr->from_state == from_id && curr->symbol.kind == asym.kind && curr->symbol.no == asym.no )
			return curr->to_state;
		curr = curr->link ;
	} // while

	return -1;
} // find_arc_in_arc_list ( )

int lookUp_nonterminal( char *str) {
	int i ;
	for (i = 0; i < Max_nonterminal; i++ )
		if (strcmp(str, Nonterminals_list[i].str) == 0)
			return i;
	return -1;
}

int lookUp_terminal(char *str) {
	int i ;
	for (i = 0; i < Max_terminal ; i++)
		if (strcmp(str, Terminals_list[i].str) == 0)
			return i;
	return -1;
}

void read_grammar(char fileName[50]) {
	FILE *fps;
	char line[500]; // line buffer
	char symstr[10];
	char *ret;
	int i, k, n_sym, n_rule, i_leftSymbol, i_rightSymbol, i_right, synkind;
	fps = fopen(fileName, "r" );
	if (!fps) {
		printf("File open error of grammar file.\n");
		getchar(); // make program pause here.
	}

	ret = fgets(line, 500, fps); // ignore line 1
	ret = fgets(line, 500, fps); // ignore line 2
	ret = fgets(line, 500, fps); // read nonterminals line.
	i = 0; n_sym = 0;
	do { // read nonterminals.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		if (line[i] == '\n') break;
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
		{ symstr[k] = line[i]; i++; k++; }
		symstr[k] = '\0'; // a nonterminal string is finished.
		strcpy(Nonterminals_list[n_sym].str, symstr);
		Nonterminals_list[n_sym].kind = 1; // nonterminal.
		Nonterminals_list[n_sym].no = n_sym;
		n_sym++;
	} while (1);
	Max_nonterminal = n_sym;

	i = 0; n_sym = 0;
	ret = fgets(line, 500, fps); // read terminals line.
	do { // read terminals.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		if (line[i] == '\n') break;
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
		{ symstr[k] = line[i]; i++; k++; }
		symstr[k] = '\0'; // a terminal string is finished.
		strcpy(Terminals_list[n_sym].str, symstr);
		Terminals_list[n_sym].kind = 0; // terminal.
		Terminals_list[n_sym].no = n_sym;
		n_sym++;
	} while (1);
	Max_terminal = n_sym;
	printf("Number of terminals and nonterminals : %d,  %d\n", Max_terminal, Max_nonterminal);

	ret = fgets(line, 500, fps); // ignore one line.
	n_rule = 0;
	do { // reading rules.
		ret = fgets(line, 500, fps);
		if (!ret)
			break; // no characters were read. So reading rules is terminated.

		// if the line inputed has only white spaces, we should skip this line.
		// this is determined as follows: length==0; or first char is not an alphabet.
		i = 0;
		if (strlen(line) < 1)
			continue;
		else {
			while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
			if ( ! isalpha(line[i]) )
				continue;
		}

		// take off left symbol of a rule.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
		{ symstr[k] = line[i]; i++; k++; }
		symstr[k] = '\0'; // a nonterminal string is finished.
		i_leftSymbol = lookUp_nonterminal(symstr);
		if (i_leftSymbol < 0) {
			printf("Wrong left symbol of a rule.\n");
			getchar();
		}

		// left symbol is stored of the rule.
		rules[n_rule].leftside.kind = 1; rules[n_rule].leftside.no = i_leftSymbol;
		strcpy(rules[n_rule].leftside.str , symstr);

		// By three lines below, we move to first char of first sym of RHS.
		while (line[i] != '>') i++;
		i++;
		while (line[i] == ' ' || line[i] == '\t') i++;

		// Collect the symbols of the RHS of the rule.
		i_right = 0;
		do { // reading symbols of RHS
			k = 0;
			while ( i < strlen(line) && (line[i] != ' '
				&& line[i] != '\t' && line[i] != '\n') )
			{ symstr[k] = line[i]; i++; k++; }
			symstr[k] = '\0';
			if (strcmp(symstr, "epsilon") == 0) { // this is epsilon rule.
				rules[n_rule].rleng = 0; // declare that this rule is an epsilon rule.
				break;
			}

			if (isupper(symstr[0])) { // this is nonterminal
				synkind = 1;
				i_rightSymbol = lookUp_nonterminal(symstr);
			}
			else { // this is terminal
				synkind = 0;
				i_rightSymbol = lookUp_terminal(symstr);
			}

			if (i_rightSymbol < -1) {
				printf("Wrong right symbol of a rule.\n");
				getchar();
			}

			rules[n_rule].rightside[i_right].kind = synkind;
			rules[n_rule].rightside[i_right].no = i_rightSymbol;
			strcpy(rules[n_rule].rightside[i_right].str, symstr);

			i_right++;

			while (line[i] == ' ' || line[i] == '\t') i++;
			// 다음 문장에서: i >= strlen(line) is needed in case of eof just after the last line.
			if (i >= strlen(line) || line[i] == '\n')
				break; // finish reading  righthand symbols.
		} while (1); // loop of reading symbols of RHS.

		rules[n_rule].rleng = i_right;
		n_rule++;
	} while (1); // loop of reading rules.

	Max_rules = n_rule;
	printf("Total number of rules = %d\n", Max_rules);
} // read_grammar.

// return 1 if an item (2nd parameter) is in the item list (1st parameter). Otherwise, return 0.
int is_item_in_itemlist ( ty_ptr_item_node item_list, ty_ptr_item_node item ) {
	ty_ptr_item_node curr = item_list;

	while(curr){
		if(curr->RuleNum == item->RuleNum && curr->DotNum == item->DotNum)
			return 1;
		else
			curr = curr->link;
	} // while : cursor

	return 0;
} // is_item_in_itemlist ()

// return the pointer to the last item node of an item list.
ty_ptr_item_node	getLastItem ( ty_ptr_item_node it_list ) {
	ty_ptr_item_node curr = it_list;

	while(curr){
		if(!curr->link)
			return curr;
		else
			curr = curr->link;
	} // while

	return NULL;
} // getLastItem ()

void	itemListPrint(ty_ptr_item_node IS){
	int i_0;
	int r_num, d_num;

	ty_ptr_item_node cursor = IS;

	while(cursor){
		r_num = cursor->RuleNum;
		d_num = cursor->DotNum;

		printf("%s -> ", Nonterminals_list[rules[r_num].leftside.no].str );
		for(i_0 = 0; i_0 < rules[r_num].rleng; i_0++){
			if(i_0 == d_num)
				printf(". ");
			printf("%s ", rules[r_num].rightside[i_0].kind ? Nonterminals_list[rules[r_num].rightside[i_0].no].str :
				Terminals_list[rules[r_num].rightside[i_0].no].str );
		} // for : i_0
		if(i_0 == d_num)
			printf(".");
		if(!rules[r_num].rleng)
			printf("ε");
		printf("     ");
		cursor = cursor->link;
	} // while : cursor
	printf("\n");
} // itemListPrint ( )

// print items of a item list into a file.
void	fitemListPrint(ty_ptr_item_node IS, FILE *fpw ){
	int i_0;
	int r_num, d_num;

	ty_ptr_item_node cursor = IS;

	while (cursor){
		r_num = cursor->RuleNum;
		d_num = cursor->DotNum;

		fprintf(fpw, "%s -> ", Nonterminals_list[rules[r_num].leftside.no].str);
		for (i_0 = 0; i_0 < rules[r_num].rleng; i_0++){
			if (i_0 == d_num)
				fprintf(fpw, ". ");
			fprintf(fpw, "%s ", rules[r_num].rightside[i_0].kind ? Nonterminals_list[rules[r_num].rightside[i_0].no].str :
				Terminals_list[rules[r_num].rightside[i_0].no].str);
		} // for : i_0
		if (i_0 == d_num)
			fprintf(fpw, ".");
		if (!rules[r_num].rleng)
			fprintf(fpw, "ε");
		fprintf(fpw, "     ");
		cursor = cursor->link;
	} // while : cursor
	fprintf(fpw, "\n");
} // fitemListPrint ( )

// IS 가 가리키는 item list 의 closure 를 구하여 IS 를 확장한다.
// 주의:  결과적으로 IS 가 변경될 수 있다.
ty_ptr_item_node closure(ty_ptr_item_node IS) {
	ty_ptr_item_node	new_item = NULL;
	ty_ptr_item_node	curr = NULL;
	ty_ptr_item_node	last_item = NULL;

	sym		SymAfterDot;

	int	r_num, d_num;
	int	i_0;

	// 포인터 curr 가 IS 의  첫 노드를 가리키게 한다.
	curr = IS;

	// Core Routine
	while (curr){
		// curr 노드의 rule 번호
		r_num = curr->RuleNum;
		d_num = curr->DotNum;

		// SymAfterDot : type.sym { int kind / int no }
		if (d_num >= rules[r_num].rleng) {	// dot 다음에 아무 심볼도 없는 경우.
			curr = curr->link;	// 다음 item 으로 이동.
			continue;
		} else
			SymAfterDot = rules[r_num].rightside[d_num];	// dot 다음의 심볼

		if (!SymAfterDot.kind){		// 단말기호이면 무시하고 넘어간다.
			curr = curr->link;
			continue;
		}

		// SymAfterDot 이 비단말기호이다. SymAfterDot 을 좌측심볼로 가지는 룰마다 아이템을 추가한다.
		for (i_0 = 0; i_0 < Max_rules; i_0++) {
			// 룰 i_0 의 좌측 심볼이 SymAfterDot 이 아니면 이 룰은 무시한다.
			if (rules[i_0].leftside.no != SymAfterDot.no)
				continue;

			// item node 하나를 만든다.
			new_item = (ty_ptr_item_node)malloc(sizeof(ty_item_node));

			// rule 번호 r, dot number = 0 을 여기에 넣는다.
			new_item->RuleNum = i_0;
			new_item->DotNum = 0;
			new_item->link = NULL;

			// new_item 이 지금까지의 closure 결과에 이미 존재하면 추가하지 않는다.
			if (is_item_in_itemlist(IS, new_item)){
				free(new_item);
				continue;
			} // if : is_item_in_itemlist ()

			//NewItemNodePtr 를 IS 의 맨 뒤에 붙인다.
			last_item = getLastItem(IS);
			last_item->link = new_item;

		} // for : i_0
		curr = curr->link;
	} // while : curr

	return IS;
} // closure.

// Goto function. Compute the item list which can be reached from IS  by  sym_val.
ty_ptr_item_node	goto_function( ty_ptr_item_node IS, sym sym_val ){
	int r_num, d_num ;
	sym	SymAfterDot ;
	ty_ptr_item_node curr_item;
	ty_ptr_item_node New_Item;
	ty_ptr_item_node Go_To_Result_List  = NULL;
	ty_ptr_item_node i_cursor  = NULL;
	ty_ptr_item_node temp_item = NULL;

	curr_item = IS;
	while (curr_item) {		// 인수로 받은 item list IS 내의 모든 item 마다 처리해 준다.
		r_num = curr_item->RuleNum ;
		d_num = curr_item->DotNum ;

		if ( d_num >= rules[r_num].rleng ) 		{	// dot 이 맨 끝에 있음. 이 아이템은 무시.
			curr_item = curr_item->link;
			continue ;
		}

		SymAfterDot = rules[r_num].rightside[d_num];  // dot 바로 뒤 심볼.

		// 점 다음의 심볼과 goto할 심볼이 다르면 다음 아이템으로 넘어 감.
		if (!(SymAfterDot.kind == sym_val.kind && SymAfterDot.no == sym_val.no) ) 	{
			curr_item = curr_item->link;
			continue;
		}

		// curr_item 이 A->apha . X beta 이고, X == sym_val 이므로,
		// A->apha X . beta 을 추가해야 함.

		New_Item = (ty_ptr_item_node)malloc(sizeof(ty_item_node)) ;	// 새 item 을 만든다
		New_Item->RuleNum = r_num ;
		New_Item->DotNum  = d_num + 1 ;
		New_Item->link    = NULL ;

		// New_Item 가 이미 존재하면, 넣지 않고 이 아이템 curr_item 처리는 무시한다.
		if ( is_item_in_itemlist ( Go_To_Result_List, New_Item ) ){
			free(New_Item);
			curr_item = curr_item->link;
			continue;
		} // if : is_item_in_itemlist ()

		//NewItemNodePtr 를 Go_To_Result_List 의 맨 뒤에 붙인다.
		if (Go_To_Result_List == NULL) {
			Go_To_Result_List = New_Item;
		} else {
			temp_item = getLastItem( Go_To_Result_List ) ;
			temp_item->link = New_Item;		// 마지막 item 이 되도록 붙인다.
		} // if

		curr_item = curr_item->link;
	} // while

	if (Go_To_Result_List)
		return closure( Go_To_Result_List ) ;
	else
		return NULL;
} // goto_function ( )

// 모든 비단말기호의 first 를 구하는 함수. 함수 first 호출하여 계산한다.
void first_all() {
	int i, j, r, m, A, k, n, Xno;
	sym X, Y;

	// 먼저 모든 비단말기호들의 first 를 구하여 First_table 에 기록한다.
	for (i = 0; i < Max_nonterminal; i++) {
		X = Nonterminals_list[i];
		if (done_first[i] == 0) { // 아직 이 비단말기호를 처리하지 않음.
			top_ch_stack = -1;
			first(X);
		}
		if (top_ch_stack != -1) {
			printf("Logic error. stack top should be -1.\n");
			getchar();  // 일단 멈추게 함.
		}
	} // for

	// 재계산점 처리
	int change_occurred;
	type_recompute recom;

	while (1) {
		change_occurred = 0;
		for (m = 0; m < num_recompute; m++) {
			recom = recompute_list[m];
			r = recom.r; Xno = recom.X; i = recom.i; A = recom.Yi;
			k = rules[r].rleng;
			for (j = i; j < k; j++) {
				Y = rules[r].rightside[j];
				if (Y.kind == 0) {  // 단말기호이면,
					First_table[Xno][Y.no] = 1;
					break;  // 규칙 r 의 처리 종료하고, 다음 재계산점으로 간다.
				}
				// 여기 오면 Y는 비단말기호임. Y 의 first 를 모두 X 의 first로 등록한다(입실론 제외).
				for (n = 0; n < Max_terminal; n++) {
					if (First_table[Y.no][n] == 1) {
						if (First_table[Xno][n] == 0)
							change_occurred = 1; // 0 인 것이 1 로 변경됨. 즉 first 가 추가가 됨.
						First_table[Xno][n] = 1;
					}  // if
				}  // for(n=0
				if (First_table[Y.no][Max_terminal] == 0)	// Y 의 first 에 입실론이 없다면,
					break;
			} // for (j=i
			if (j == k) {  // 이것이 참이면, 룰의 우측의 모든 심볼이 입실론을 가짐.
				if (First_table[Xno][Max_terminal] == 0)
					change_occurred = 1;
				First_table[Xno][Max_terminal] = 1;  // 입실론을 넣어 줌.
			} // if
		} // for (m=0
		if (change_occurred == 0)
			break;	// 재계산점 처리에서 변화를 주지 못함.
	} // while
} // end of first_all

// Is nonterminal Y in recompute list?
int is_nonterminal_in_stoplist(int Y) {
	int i;
	for (i = 0; i < num_recompute; i++) {
		if (recompute_list[i].X == Y)
			return 1;
	}
	return 0;
}  // end of is_nonterminal_in_stoplist

// is a nonterminal in ch_stack?
int nonterminal_is_in_stack(int Y) {
	int i;
	if (top_ch_stack >= 0) {
		for (i = 0; i <= top_ch_stack; i++)
			if (ch_stack[i] == Y)
				return 1;
	}
	return 0;
} // end of nonterminal_is_in_stack

//  first computation of one nonterminal with capability of dealing with case-2.
int first(sym X) {              // assume X is a nonterminal.
	int i, j, k, r, rleng;
	sym Yi;
	int Yi_has_epsilon;

	// push to the stack.
	top_ch_stack++;
	ch_stack[top_ch_stack] = X.no;

	for (r = 0; r < Max_rules; r++) {
		if (rules[r].leftside.no != X.no)
			continue; // skip this rule since left side is not X.
		rleng = rules[r].rleng;
		if (rleng == 0) {
			First_table[X.no][Max_terminal] = 1; // 입실론을 first 에  추가.
			continue;  // 다음 룰로 간다.
		}

		for (i = 0; i < rleng; i++) {
			Yi = rules[r].rightside[i];	// Yi 는 룰의 우측편의 위치 i 의 심볼
			if (Yi.kind == 0) {	// Yi is terminal
				First_table[X.no][Yi.no] = 1;	// Yi를 X의 first 로 추가.
				break;	// exit this loop to go to next rule.
			}
			// Now, Yi is nonterminal.
			if (X.no == Yi.no) {
				if (First_table[X.no][Max_terminal] == 1)
					continue;  // Yi 우측편을 처리하게 함.
				else
					break; // Yi 의 우측은 무시하고 다음 룰로 간다.
			}

			if (done_first[Yi.no] == 1) {	// if done_first of Yi is 1
				if (is_nonterminal_in_stoplist(Yi.no)) {	// Yi 가 좌심볼인 재계산점이 존재하면,
					a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
					recompute_list[num_recompute] = a_recompute;	// 재계산점 [r,X,i,Yi] 를 넣는다.
					num_recompute++;
					// 그리고 아래 ||A|| 로 간다.
				}
			}
			else {	// done_first of Yi == 0이다.
				if (nonterminal_is_in_stack(Yi.no)) {	// Yi 가 ch_stack 에 있다면
					a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
					recompute_list[num_recompute] = a_recompute;	// 재계산점 [r,X,i,Yi] 를 넣는다.
					num_recompute++;
					// 그리고 아래 ||A|| 로 간다.
				}
				else {	// Yi 의 first 를 계산한 적이 없다.
					first(Yi);	// Yi 의 first 를 계산하여 First_table 에 등록함.
					if (is_nonterminal_in_stoplist(Yi.no)) {	// Yi가 좌심볼인 재계산점이 있다면,
						a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
						recompute_list[num_recompute] = a_recompute;	// 재계산점 [r,X,i,Yi] 를 넣는다.
						num_recompute++;
					}
				}  // else
			} // else
			// ||A||: Yi 의 비-입실론 first 를 first_X 에 반영. 입실론이 있다면 다음 심볼 처리로 아니면 다음 룰 처리로 간다.
			int n;
			for (n = 0; n < Max_terminal; n++) {
				if (First_table[Yi.no][n] == 1)
					First_table[X.no][n] = 1;
			}
			if (First_table[Yi.no][Max_terminal] == 0)	// 입실론이 Yi 의 first 에 없으므로.
				break; // 다음 룰로 간다.
		} // for (i=0
		// break 의 수행으로 여기로 온다면 i != rleng 이다. 그렇지 않다면 i==rleng 이고,
		// 이 말은 r 규칙의 우측편 모두가 입실론을 가지고 있다. 따라서 X 도 입실론을 가져야 한다.
		if (i == rleng)
			First_table[X.no][Max_terminal] = 1;  // X 의 first 가 입실론을 가지게 한다.
	} // for (r=0

	done_first[X.no] = 1;  // X 의 done_first (first 계산완료)을 1 로 한다.

	// pop stack.
	ch_stack[top_ch_stack] = -1;  // dummy 값을 넣는다.
	top_ch_stack--;

	return 1;
} // end of first

// alpha is an arbitrary string of terminals or nonterminals.
// A dummy symbol with kind = -1 must be place at the end as the end marker.
// length: number of symbols in alpha.
void first_of_string(sym alpha[], int length, int first_result[]) {
	int i, k;
	sym Yi;
	for (i = 0; i < Max_terminal + 1; i++)
		first_result[i] = 0; // initialize the first result of alpha

	// Let alpha be Y0 Y1 ... Y(length-1)

	for (i = 0; i < length; i++) {
		Yi = alpha[i];
		if (Yi.kind == 0) {  // Yi is terminal
			first_result[Yi.no] = 1;
			break;
		}
		else {  // 여기 오면 Yi 는 비단말기호이다.
			for (k = 0; k < Max_terminal; k++)	 // copy first of Yi to first of alpha
				if (First_table[Yi.no][k] == 1) first_result[k] = 1;
			if (First_table[Yi.no][Max_terminal] == 0)
				break; // first of Yi does not have epsilon. So forget remaining part.
		}
	} // for
	if (i == length)
		first_result[Max_terminal] = 1;  // epsilon 을 넣는다.
} // end of function first_of_string

// This function computes the follow of a nonterminal with index X.
int follow_nonterminal(int X) {
	int i, j, k, m;
	int first_result[Max_symbols]; // one row of First table.
	int leftsym;
	sym SymString[Max_string_of_RHS];

	for (i = 0; i < Max_terminal + 1; i++)
		first_result[i] = 0; // initialization.

	for (i = 0; i < Max_rules; i++) {
		leftsym = rules[i].leftside.no;
		for (j = 0; j < rules[i].rleng; j++)
		{    //  the symbol of index j of the RHS of rule i is to be processed in this iteration
			if (rules[i].rightside[j].kind == 0 || rules[i].rightside[j].no != X)
				continue; // not X. so skip this symbol j.

			// Now, position j has a nonterminal which is equal to X.
			if (j < rules[i].rleng - 1) {  // there are symbols after position j in RHS of rule i.
				m = 0;
				for (k = j + 1; k < rules[i].rleng; k++, m++) SymString[m] = rules[i].rightside[k];
				SymString[m].kind = -1;  // end of string marker.
				first_of_string(SymString, m, first_result); // Compute the first of the string after position j of rule i.
																		   // the process result is received via first_result.
				for (k = 0; k < Max_terminal; k++) // Copy the first symbols of the remaining string(except eps) to the Follow of X.
					if (first_result[k] == 1)
						Follow_table[X][k] = 1;
			} // if

			if (j == rules[i].rleng - 1 || first_result[Max_terminal] == 1) // j is last symbol or first result has epsilon
			{
				if (rules[i].leftside.no == X)
					continue; // left symbol of this rule == X. So no need of addition.
				if (done_follow[leftsym] == 0) // the follow set of the left sym of rule i was not computed.
					follow_nonterminal(leftsym);		// compute it.
				for (k = 0; k < Max_terminal; k++) // add follow terminals of left side symbol to follow of X (without epsil).
					if (Follow_table[leftsym][k] == 1)
						Follow_table[X][k] = 1;
			}
		} // end of for j=0.
	} // end of for i
	done_follow[X] = 1;  // put the completion mark for this nonterminal.
	return 1;
} // end of function follow_nonterminal.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
	int i, j, base_x, base_y;
	sym a_nonterminal = { 1, 0 };
	char grammar_file_name[50];

	//read_grammar("G_arith_no_LR.txt");
	//read_grammar("G_arith_with_LR.txt");
	strcpy(grammar_file_name, "Grammar_data.txt");
	//strcpy(grammar_file_name, "G_arith_with_LR.txt");
	//strcpy(grammar_file_name, "G_arith_no_LR.txt");
	//strcpy(grammar_file_name, "G_case1.txt");
	strcpy(grammar_file_name, "G_arbi_1.txt");
	read_grammar(grammar_file_name);
	strcpy(Terminals_list[Max_terminal].str, "Epsilon");

	// initialze First table.
	for (i = 0; i < Max_nonterminal; i++) {
		for (j = 0; j < (Max_terminal + 1); j++) {   // 0 ~ Max_terminal including epsilon.
			First_table[i][j] = 0;
		}
		done_first[i] = 0;
	}

	// initialze Follow table.
	for (i = 0; i < Max_nonterminal; i++) {
		for (j = 0; j < Max_terminal; j++) {  // 0 ~ (Maxterminal-1)
			Follow_table[i][j] = 0;
		}
		done_follow[i] = 0;
	}

	// Compute first of all nonterminals
	first_all();

	// Compute follow of all nonterminals
	Follow_table[0][Max_terminal - 1] = 1; // make $ to be a follow symbol of the starting nonterminal.
										   // Index of $ is the last index which is (Max_terminal-1) !!

	for (i = 0; i < Max_nonterminal; i++) {
		if (done_follow[i] == 0) // if follow computation of nonterminal i was not done_first, compute it.
			follow_nonterminal(i);
	}

	// Print first of all nonterminals.
	for (i = 0; i < Max_nonterminal; i++) {
		printf("First(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < Max_terminal; j++) {
			if (First_table[i][j] == 1)
				printf("%s  ", Terminals_list[j].str);
		}
		if (First_table[i][Max_terminal] == 1)
			printf(" eps");
		printf("\n");
	}

	// Print follow of all nonterminals.
	printf("\n");
	for (i = 0; i < Max_nonterminal; i++) {
		printf("Follow(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < Max_terminal; j++) {
			if (Follow_table[i][j] == 1)
				printf("%s  ", Terminals_list[j].str);
		}
		printf("\n");
	}

	ty_ptr_item_node	ItemSet0, tptr;

	tptr = (ty_ptr_item_node)malloc(sizeof(ty_item_node));
	tptr->RuleNum = 0;
	tptr->DotNum = 0;
	tptr->link = NULL;

	ItemSet0 = closure(tptr);  // This is state 0.

	make_goto_graph(ItemSet0);
	printGotoGraph(Header_goto_graph);

	printf("Program terminates.\n");
} // main.
