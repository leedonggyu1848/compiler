// 컴파일러 실습 프로그램:  first, follow 구하기.

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define Max_symbols 100 // Number of terminals and nonterminals must be smaller than this number.
#define Max_size_rule_table 100  //  actual number of Rules must be smaller than this number.
#define Max_string_of_RHS 20	 // the maximum number of symbols of right side of Rules

typedef struct ssym {
	 int kind ; // 0 if terminal; 1 if nonterminal; -1 if dummy symbol indicating the end mark.
	 int no ; // the unique index of a symbol.
	 char str[10]; // string of the symbol
} sym ;  // definition symbols

// Rules are represented as follows
typedef struct orule {  // rule 한 개의 정의.
    sym leftside ;
    sym rightside [Max_string_of_RHS] ;
    int  rleng ;  // RHS 의 심볼의 수.  0 is given if righthand side is epsilon.
} onerule ;

typedef struct {
	int r, X, i, Yi;
} type_recompute;

int Max_terminal;
int Max_nonterminal;
sym Nonterminals_list [Max_symbols];
sym Terminals_list [Max_symbols];

int Max_rules;  // 룰의 총 갯수를 가진다.
onerule Rules[Max_size_rule_table];	// 룰 들을 가진다.

int First_table[Max_symbols][Max_symbols]; // actual region:  Max_nonterminal  X (MaxTerminals+2)
int Follow_table[Max_symbols][Max_symbols]; // actual region:  Max_nonterminal  X (MaxTerminals+1)
int done_first[Max_symbols];	// 비단말기호의 first 계산 완료 플래그
int done_follow[Max_symbols];  // 비단말기호의 follow 계산 완료 플래그

type_recompute recompute_list[500];    // recompute_list point list
int num_recompute; // number of recompute_list points in recompute_list.
type_recompute a_recompute;

int ch_stack[100] = { -1, }; //call history stack. -1 은 더미 값. 초기화 안해도 상관없다.
int top_stack = -1;

int lookUp_nonterminal( char *str) {
	int i ;
	for (i = 0; i < Max_nonterminal; i++ ) {
		if (strcmp(str, Nonterminals_list[i].str) == 0)
			return i;
	}
	return -1;
}

int lookUp_terminal(char *str) {
	int i ;
	for (i = 0; i < Max_terminal ; i++)
		if (strcmp(str, Terminals_list[i].str) == 0)
			return i;
	return -1;
}

// 입력파일에 넣을 grammar 정보 내용은 강의 중에 설명함.
void read_grammar(char *fileName) {
	FILE *fp;
	char line[500]; // line buffer
	char symstr[10];
	char *ret;
	int i, k, n_sym, n_rule, i_leftSymbol, i_rightSymbol, i_right, synkind;
	fp = fopen(fileName, "r" );
	if (!fp) {
		printf("File open error of grammar file.\n");
		getchar(); // make program pause here.
	}

	ret = fgets(line, 500, fp); // ignore line 1
	ret = fgets(line, 500, fp); // ignore line 2
	ret = fgets(line, 500, fp); // read nonterminals line.
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
	ret = fgets(line, 500, fp); // read terminals line.
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

	ret = fgets(line, 500, fp); // ignore one line.
	n_rule = 0;
	do { // reading Rules.
		ret = fgets(line, 500, fp);
		if (!ret)
			break; // no characters were read. So reading Rules is terminated.

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
		Rules[n_rule].leftside.kind = 1; Rules[n_rule].leftside.no = i_leftSymbol;
		strcpy(Rules[n_rule].leftside.str , symstr);

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
				Rules[n_rule].rleng = 0; // declare that this rule is an epsilon rule.
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

			Rules[n_rule].rightside[i_right].kind = synkind;
			Rules[n_rule].rightside[i_right].no = i_rightSymbol;
			strcpy(Rules[n_rule].rightside[i_right].str, symstr);

			i_right++;

			while (line[i] == ' ' || line[i] == '\t') i++;
			if (i >= strlen(line) || line[i] == '\n') // i >= strlen(line) is needed in case of eof just after the last line.
				break; // finish reading  righthand symbols.
		} while (1); // loop of reading symbols of RHS.

		Rules[n_rule].rleng = i_right;
		n_rule++;
	} while (1); // loop of reading Rules.

	Max_rules = n_rule;
	printf("Total number of Rules = %d\n", Max_rules);
} // read_grammar.

// Is nonterminal Y in recompute list?
int is_nonterminal_in_recompute_list(int Y) {
	int i;
	for (i = 0; i < num_recompute; i++) {
		if (recompute_list[i].X == Y)
			return 1;
	}
	return 0;
}  // end of is_nonterminal_in_recompute_list

// is a nonterminal in ch_stack?
int nonterminal_is_in_stack(int Y) {
	int i;
	if (top_stack >= 0) {
		for (i = 0; i <= top_stack; i++)
			if (ch_stack[i] == Y)
				return 1;
	}
	return 0;
} // end of nonterminal_is_in_stack

//  first computation of one nonterminal with capability of dealing with case-2.
void first (sym X) {              // assume X is a nonterminal.
	int i, r, rleng;
	sym Yi;

	top_stack++;
	ch_stack[top_stack] = X.no;  // push to the stack.

	for (r = 0; r < Max_rules; r++) {
		if (Rules[r].leftside.no != X.no)
			continue; // skip this rule since left side is not X.
		rleng = Rules[r].rleng;
		if (rleng == 0) {
			First_table[X.no][Max_terminal] = 1; // 입실론을 first 에  추가.
			continue;  // 다음 룰로 간다.
		}

		for (i = 0; i < rleng; i++) {
			Yi = Rules[r].rightside[i];	// Yi 는 룰의 우측편의 위치 i 의 심볼
			if (Yi.kind == 0) {	// Yi is terminal
				First_table [X.no][Yi.no] = 1;	// Yi를 X의 first 로 추가.
				break;	// exit this loop to go to next rule.
			}
			// Now, Yi is nonterminal.
			if (X.no == Yi.no) {	// case-1 발생함.
				printf("Case 1  has occurred: rule: %d, position of RHS:%d\n", r, i);
				if (First_table[X.no][Max_terminal] == 1) {
					continue;  // epsilon 이 first of X 에 있으므로 Yi 우측편을 처리하게 함.
				}
				else
					break; // epsilon 이 first of X 에 없으므로 Yi의 우측은 무시하고 다음 룰로 간다.
			}

			if (done_first[Yi.no] == 1) {	// Yi 의 first 계산을 이미 마쳤음.
				if (is_nonterminal_in_recompute_list(Yi.no)) {	// Yi 가 좌심볼인 재계산점이 존재하면,
					a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
					recompute_list[num_recompute] = a_recompute;	// 재계산점 [r,X,i,Yi] 를 넣는다.
					num_recompute++;
					printf("재계산점 추가:[%d,%s,%d,%s]\n", r, Nonterminals_list[X.no].str, i, Nonterminals_list[Yi.no].str);
				}
			}
			else {	// done of Yi == 0이다. 즉 Yi 의 first 계산은 아직 안했음.
				if (nonterminal_is_in_stack(Yi.no)) {	// Yi 가 ch_stack 에 있다면
					a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
					recompute_list[num_recompute] = a_recompute;	// 재계산점 [r,X,i,Yi] 를 넣는다.
					num_recompute++;
					printf("재계산점 추가:[%d,%s,%d,%s]\n", r, Nonterminals_list[X.no].str, i, Nonterminals_list[Yi.no].str);
				}
				else {	// Yi 가 ch_stack 에 없다.
					first(Yi);	// Yi 의 first 를 계산하여 First_table 에 등록함.
					if (is_nonterminal_in_recompute_list(Yi.no)) {	// Yi가 좌심볼인 재계산점이 있다면,
						a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
						recompute_list[num_recompute] = a_recompute;	// 재계산점 [r,X,i,Yi] 를 넣는다.
						num_recompute++;
						printf("재계산점 추가:[%d,%s,%d,%s]\n", r,Nonterminals_list[X.no].str,i, Nonterminals_list[Yi.no].str);
					}
				}  // else
			} // else
			// Yi 의 first 의 원소 중에 epsilon 이 아닌 것들을 first_X 에 넣어준다.
			int n;
			for (n = 0; n < Max_terminal; n++) {
				if (First_table[Yi.no][n] == 1)
					First_table[X.no][n] = 1;
			}
			// epsilon이 Yi 의 first 에 없다면 다음 룰로 간다. 있다면 Yi 의 우측 편 심볼로 간다.
			if (First_table[Yi.no][Max_terminal] == 0)
				break;
		} // for (i=0
		// break 의 수행으로 여기로 온다면 i != rleng 이다. 그렇지 않다면 i==rleng 이고,
		// 이 말은 r 규칙의 우측편의 모든 심볼의 first 가 epsilon을 가진다. 고로 X 도 가져야 한다.
		if (i == rleng)
			First_table[X.no][Max_terminal] = 1;  // X 의 first 가 epsilon을 가지게 한다.
	} // for (r=0

	done_first[X.no] = 1;  // X 의 done (first 계산완료)을 1 로 한다.

	// pop stack.
	ch_stack[top_stack] = -1;  // dummy 값을 넣는다. 사실 이 줄은 안해도 됨!
	top_stack--;  // 이 줄이 실제로 pop 을 하는 것임.
} // end of first

// alpha is an arbitrary string of terminals or nonterminals.
// A dummy symbol with kind = -1 must be placed at the end as the end marker.
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

// 모든 비단말기호의 first 를 구한다. (재계산점 처리도 수행하여 case-2까지 처리함.)
void first_all() {
	int i, j, r, m, A, k, n, Xno;
	sym X, Y;

	// 먼저 모든 비단말기호들의 first 를 구하여 First_table 에 기록한다.
	for (i = 0; i < Max_nonterminal; i++) {
		X = Nonterminals_list[i];
		if (done_first[i] == 0) { // 아직 이 비단말기호를 처리하지 않음.
			top_stack = -1; // 스택을 비워 줌. (이 줄은 사실은 안해도 됨.)
			first(X);
		}
		if (top_stack != -1) {
			printf("Logic error. stack top should be -1.\n");
			getchar();  // 일단 멈추게 함.
		}
	} // for

	// 재계산점 처리
	int change_occurred;
	type_recompute recom;

	while (1) {
		// 루프 한번 돌 때, 모든 재계산점들을 처리한다.
		change_occurred = 0;
		for (m = 0; m < num_recompute; m++) {
			recom = recompute_list[m];
			r = recom.r; Xno = recom.X; i = recom.i; A = recom.Yi;
			k = Rules[r].rleng;
			for (j = i; j < k; j++) {
				Y = Rules[r].rightside[j];
				if (Y.kind == 0) {  // 단말기호이면,
					if (First_table[Xno][Y.no] == 0) {
						change_occurred = 1;
						printf("%s is added to first of %s in recomputing\n", Y.str, Nonterminals_list[Xno].str);
					}
					First_table[Xno][Y.no] = 1;
					break;  // 규칙 r 의 처리 종료하고, 다음 재계산점으로 간다.
				}
				// 여기 오면 Y는 비단말기호임. Y 의 first 를 모두 X 의 first로 등록한다(입실론 제외).
				for (n = 0; n < Max_terminal; n++) {
					if (First_table[Y.no][n] == 1) {
						if (First_table[Xno][n] == 0) {
							change_occurred = 1;
							printf("%s is added to first of %s in recomputing\n", Terminals_list[n].str, Nonterminals_list[Xno].str);
						}
						First_table[Xno][n] = 1;
					}  // if
				}  // for(n=0
				if (First_table[Y.no][Max_terminal] == 0)	// Y 의 first 에 입실론이 없다면,
					break;  // Y 우측은 무시.
			} // for (j=i
			if (j == k) {  // 이것이 참이면, 룰의 우측의 모든 심볼이 입실론을 가짐.
				if (First_table[Xno][Max_terminal] == 0) {
					change_occurred = 1;
					printf("epsilon is added to first of %s in recomputing\n", Nonterminals_list[Xno].str);
				}
				First_table[Xno][Max_terminal] = 1;  // 입실론을 넣어 줌.
			} // if
		} // for (m=0
		if (change_occurred == 0)
			break;	// 재계산점 처리에서 전혀 변화가 없었음.
	} // while
} // end of first_all

// This function computes the follow of a nonterminal with index X.
int follow_nonterminal ( int X ) {
   int i, j, k, m ;
   int first_result[Max_symbols]; // one row of First table.
   int leftsym;
   sym SymString [Max_string_of_RHS] ;

   for (i = 0; i < Max_terminal + 1; i++)
	   first_result[i] = 0; // initialization.

   for (i = 0; i < Max_rules; i++) {
	   leftsym = Rules[i].leftside.no;
       for ( j=0; j < Rules[i].rleng ; j++ )
       {    //  the symbol of index j of the RHS of rule i is to be processed in this iteration
              if ( Rules[i].rightside[j].kind == 0 || Rules[i].rightside[j].no != X )
				  continue ; // not X. so skip this symbol j.
              // Now, position j has a nonterminal which is equal to X.
              if ( j < Rules[i].rleng - 1 ) {  // there are symbols after position j in RHS of rule i.
                     m = 0 ;
                     for (k= j+1; k < Rules[i].rleng; k++, m++ ) SymString [m] = Rules[i].rightside[k] ;
                     SymString[m].kind = -1 ;  // end of string marker.
                     first_of_string ( SymString, m, first_result ) ; // Compute the first of the string after position j of rule i.
																				// the process result is received via first_result.
                     for ( k=0; k < Max_terminal; k++) // Copy the first symbols of the remaining string(except eps) to the Follow of X.
                        if ( first_result [k] == 1 )
							Follow_table[X][k] = 1 ;
              } // if

              if (j == Rules[i].rleng - 1 ||  first_result [Max_terminal] == 1 ) // j is last symbol or first result has epsilon
              {    if ( Rules[i].leftside.no == X )
						continue ; // left symbol of this rule == X. So no need of addition.
                   if ( Follow_table[leftsym][Max_terminal] == 0 ) // the follow set of the left sym of rule i was not computed.
                        follow_nonterminal (leftsym) ;		// compute it.
                   for ( k=0; k < Max_terminal; k++) // add follow terminals of left side symbol to follow of X (without epsil).
                       if (Follow_table[leftsym][k] == 1 )
						   Follow_table[X][k] = 1 ;
               }
       } // end of for j=0.
   } // end of for i
   Follow_table[X][Max_terminal] = 1 ;  // put the completion mark for this nonterminal.
   return 1;
} // end of function follow_nonterminal.


int main()
{
	int i, j;
	sym a_nonterminal = { 1, 0 };

	// read_grammar("G_arith_no_LR.txt");
	//read_grammar("G_arith_with_LR.txt");
	//read_grammar("G_case1.txt");
	read_grammar("G_case2.txt");
	// read_grammar("G_simple_case2.txt");

	//strcpy(Terminals_list[Max_terminal].str, "eps"); // epsilon

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

	num_recompute = 0;  // 재계산점 리스트를 비워 놓는다.

	// 모든 비단말기호에 대하여 first 를 계산한다.
	first_all();

	// Print first of all nonterminals.
	printf("\nFirst table of all nonterminals:\n");
	for (i = 0; i < Max_nonterminal; i++) {
		printf("First(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < Max_terminal; j++) {
			if (First_table[i][j] == 1)
				printf("%s  ", Terminals_list[j].str);
		}
		if (First_table[i][Max_terminal] == 1)
			printf(" eps"); // epsilon
		printf("\n");
	}

	// Compute follow of all nonterminals
	Follow_table[0][Max_terminal - 1] = 1; // make $ to be a follow symbol of the starting nonterminal.
	                                       // Index of $ is the last index which is (Max_terminal-1) !!

	for (i = 0; i < Max_nonterminal; i++) {
		if (done_follow[i] == 0) // if follow computation of nonterminal i was not done, compute it.
			follow_nonterminal(i);
	}

	// Print follow of all nonterminals.
	printf("\nFollow table of all nonterminals:\n");
	for (i = 0; i < Max_nonterminal; i++) {
		printf("Follow(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < Max_terminal; j++) {
			if (Follow_table[i][j] == 1)
				printf("%s  ", Terminals_list[j].str);
		}
		printf("\n");
	}

	printf("\nProgram terminates.\n");
} // main.
