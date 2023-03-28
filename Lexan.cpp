#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define UNK 53 // Unknown token
#define EOF_TOK 52 // token index of EOF token

 typedef struct tkt { // 하나의 토큰을 나타내는 구조체
    int index;  // index : 토큰의 종류
	  char typ[16];
	  char sub_kind [3] ;  // relational operation: 토큰 구분
	                       // number: 토큰의 양의 정수인지("in") 실수인지를("do")
	  int sub_data; // ID 토큰의 심볼테이블엔트리 번호, 양의 정수 값을 저장
	  double rnum;  // 실수
	  char str[100]; // 토큰을 구성하는 스트링
	  char string_constant[100]; // buf string
	  char char_constant; // buf char
  } tokentype ;

 typedef char oneword [50] ;

 // keyword 테이블
 // token index of "if" is 28. token index of other keywords are computed using this info.
 char keywords[19][50] =
	{ "if",    "else",  "while",  "do",   "for",    "include", "define", "typedef","struct","int",
	   "char", "float", "double", "void", "return", "case",    "then",    "true",   "false" };
 int total_keywords = 19;

 //  symbol table
 typedef struct sym { oneword idstr ; int attribute; } sym_ent;  // 심볼테이블의 한 엔트리의 구조
 sym_ent symtbl [500] ;
 int total_ids = 0 ; // 심볼테이블에서 다음에 넣을 엔트리 번호

 FILE *fp, *ofp ;

 int iswhitespace(char c){
	 if (c == ' ' || c == '\n' || c == '\t' )
		 return 1;
	 else
		 return 0;
 }

 int lookup_keyword_tbl(char *str){
	int i;
	for (i = 0; i < total_keywords; i++)
		if (strcmp(keywords[i], str) == 0)
		  return i;
	return -1;

 }

 int lookup_symtbl(char *str) {
	int i;
	for (i = 0; i < total_ids; i++)
		if (strcmp(symtbl[i].idstr, str) == 0)
			return i;
	return -1;
 }

 //*
 //-------------------------------------------------------------

void push_buf_one(char *buf, int *bp, char c) {
	buf[*bp] = c;
	(*bp)++;
	buf[*bp] = '\0';
}

tokentype lexan (void) {
   int state = 0 ;
   char c;
   char buf[50];
   int bp = 0; // bp is buffer pointer
   int upper_n ; // number 토큰에서 소숫점 위 부분 즉 정수 부분
   double fraction ; // number 토큰에서 소숫점 아래 부분
   int str_bp = 0; // str_bp string_constant pointer
   tokentype token ;
   int idx, FCNT, sign, Enum ;
   strcpy(token.typ, " ");
while ( 1 ) {
    switch (state) {
     case 0: // 초기 상태
               c = fgetc (fp) ;  // fgetc can be called even if fp is after the end of file.
			                     // calling it again still returns EOF(-1) w/o invoking error.
			   if (iswhitespace(c)) state = 0;  // this is white space.
			   else if (isalpha(c)) { push_buf_one(buf, &bp, c); state = 28; }
			   else if (isdigit(c)) { push_buf_one(buf, &bp, c); upper_n = c - '0'; state = 1; }
			   else if (c == '<') state = 2;
			   else if (c == '>') state = 32;
			   else if (c == '=') state = 35;
			   else if (c == '!') state = 38;
			   else if (c == '+') state = 3;
			   else if (c == '-') state = 4;
			   else if (c == '*') state = 52;
			   else if (c == '/') state = 8;
			   else if (c == '\\') state = 53;
			   else if (c == '%') state = 54;
			   else if (c == '.') state = 55;
			   else if (c == ',') state = 56;
			   else if (c == '(') state = 57;
			   else if (c == ')') state = 58;
			   else if (c == '{') state = 59;
			   else if (c == '}') state = 60;
			   else if (c == '[') state = 61;
			   else if (c == ']') state = 62;
			   else if (c == ':') state = 63;
			   else if (c == ';') state = 64;
			   else if (c == '"') state = 65;
			   else if (c == '\'') state = 66;
			   else if (c == '#') state = 67;
			   else if (c == '|') state = 68;
			   else if (c == '&') state = 5;
			   else if (c == EOF) state = 71;
         else {
          token.index = UNK ; // 인식할 수 없는 토큰임을 나타냄.
          return token ;
        }
               break;
	 case 1: // NUM 토큰의 소수점 위 숫자열을 받아 들이는 상태.
		 c = fgetc(fp);
		 if (isdigit(c))    { push_buf_one(buf, &bp, c); upper_n = 10 * upper_n + c - '0'; state = 1; }
		 else if (c == '.') { push_buf_one(buf, &bp, c); fraction = 0; FCNT = 0; state = 9; } // 소수점을 나왔으므로 실수를 처리하는 상태로 감.
		 else if (c == 'E') { push_buf_one(buf, &bp, c); fraction = 0; state = 16; }  // E 가 있는 exponent 처리부로 감.
		 else state = 14;
		 break;
     case 2: // 각괄호글자 < 가 나온 후의 처리를 담당하는 상태.
          c = fgetc(fp) ;
          if (c == '=') state = 30 ;
          else state = 31 ;
          break;
	 case 3: // 각괄호글자 + 가 나온 후의 처리를 담당하는 상태.
		 c = fgetc(fp);
		 if (c == '=') state = 45;
		 else if (c == '+') state = 47;
		 else state = 46;
		 break;
	 case 4: // 각괄호글자 - 가 나온 후의 처리를 담당하는 상태.
		 c = fgetc(fp);
		 if (c == '-') state = 48;
		 else if (c == '=') state = 49;
		 else if (c == '>') state = 51;
		 else state = 50;
		 break;
	 case 5: // 각괄호글자 + 가 나온 후의 처리를 담당하는 상태.
		 c = fgetc(fp);
		 if (c == '&') state = 6;
		 else state = 7;
		 break;
	 case 6:
		 token.index = 2; strcpy(token.sub_kind, "");		 strcpy(token.str, "&&");
		 return token;
	 case 7: // 토큰 & 를 리턴해주는 상태.
		 ungetc(c, fp);
		 token.index = 13;
		 return token;
	 case 8:
		 c = fgetc(fp);
		 if (c == '/') state = 74;
		 else if (c == '*') state = 75;
		 else if (c == EOF) state = 71;
		 else state = 72;
		 break;
	 case 9: // 실수의 소수점 이하를 받아 들이는 상태
		 c = fgetc(fp);
		 if (isdigit(c))  {
			 push_buf_one(buf, &bp, c);
			FCNT++; fraction = fraction + (c - '0') / pow(10.0, FCNT); state = 23; }
		 else if (c == 'E') { push_buf_one(buf, &bp, c); state = 16; }
		 else if (c == EOF)  state = 26;
		 else if (iswhitespace(c)) state = 24;
		 else state = 25;// goto unknown token state
		 break;
	 case 14: // 양의 정수 토큰을 리턴하는 상태.
		 ungetc(c, fp);
		 token.index = 1; strcpy(token.sub_kind, "in"); // 양의 정수를 나타냄.
		 token.sub_data = upper_n;
		 strcpy(token.str, buf);
		 strcpy(token.typ, "Number");
		 return token;
	 case 16:
		 c = fgetc(fp);
		 if (c == '+') { push_buf_one(buf, &bp, c); sign = 1; state = 17; }
		 else if (c == '-') { push_buf_one(buf, &bp, c); sign = -1; state = 17; }
		 else if (isdigit(c)) { push_buf_one(buf, &bp, c); sign = 1; Enum = c - '0'; state = 18; }
		 else  state = 25; // error!
		 break;
	 case 17:
		 c = fgetc(fp);
		 if (isdigit(c)) { push_buf_one(buf, &bp, c); Enum = c - '0'; state = 18; }
		 else state = 25; // error!
		 break;
	 case 18:
		 c = fgetc(fp);
		 if (isdigit(c)) { buf[bp] = c; bp++;  buf[bp] = '\0'; Enum = Enum * 10 + c - '0'; state = 18; }
		 else state = 19;
		 break;
	 case 19:
		 ungetc(c, fp);
		 token.index = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
		 token.rnum = (upper_n + fraction) * pow(10.0, sign*Enum);
		 strcpy(token.str, buf);
		 strcpy(token.typ, "Number");
		 return token;

	 case 23:
		 c = fgetc(fp);
		 if (isdigit(c)) { push_buf_one(buf, &bp, c);
			FCNT++; fraction = fraction + (c - '0') / pow(10.0, FCNT); state = 23; }
		 else if (c == 'E') { push_buf_one(buf, &bp, c); state = 16; }
		 else state = 24;
		 break;
	 case 24:
		 ungetc(c, fp);
		 token.index = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
		 token.rnum = upper_n + fraction;
		 strcpy(token.str, buf);
		 strcpy(token.typ, "Number");
		 return token;
	 case 25: // unknown token
		 ungetc(c, fp);
		 token.index = UNK;
		 return token;
	 case 26:  // do not call ungetc.
		 token.index = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
		 token.rnum = upper_n + fraction;
		 strcpy(token.str, buf);
		 strcpy(token.typ, "Number");
		 return token;
	 case 28:
		 c = fgetc(fp);
		 if (isalpha(c) || isdigit(c) || c == '_') { push_buf_one(buf, &bp, c); state = 28; }
		 else	 state = 29;
		 break;
	 case 29: // id 나 keyword
		 ungetc(c, fp);
		 strcpy(token.str, buf);
		 idx = lookup_keyword_tbl(buf); // -1 if not exist.
		 if (idx >= 0) { token.index = 31 + idx; strcpy(token.typ, "Keyword"); return token; }  // Note: first keyword has token index 30.
		 // reaches here if it is not a keyword.
		 idx = lookup_symtbl(buf); // -1 if not exist.
		 if (idx >= 0) { token.index = 0; token.sub_data = idx; strcpy(token.typ, "Identifier"); return token; }
		 // reaches here if it is not in symbol table.
		 strcpy(symtbl[total_ids].idstr, buf); total_ids++;
		 token.index = 0; // ID 토큰임을 나타냄.
		 token.sub_data = total_ids - 1; // 이 ID 가 들어 있는 심볼테이블 엔트리 번호.
		 strcpy(token.typ, "Identifier");
		 return token;
	 case 30:
		 token.index = 2; strcpy(token.sub_kind, "LE"); strcpy(token.str, "<=");
		 strcpy(token.typ, "ROP");
		 return token;
	 case 31:
		 ungetc(c, fp);
		 token.index = 2; strcpy(token.sub_kind, "LT"); strcpy(token.str, "<");
		 strcpy(token.typ, "ROP");
		 return token;
	 case 32:
		 c = fgetc(fp);
		 if (c == '=') state = 33;
		 else state = 34;
		 break;
	 case 33:
		 token.index = 2; strcpy(token.sub_kind, "GE");	strcpy(token.str, ">=");
		 strcpy(token.typ, "ROP");
		 return token;
	 case 34:
		 ungetc(c, fp);
		 token.index = 2; strcpy(token.sub_kind, "GT"); strcpy(token.str, ">");
		 strcpy(token.typ, "ROP");
		 return token;
	 case 35: // 글자 = 가 나온 후의 처리를 담당하는 상태.
		 c = fgetc(fp);
		 if (c == '=') state = 36;
		 else state = 37;
		 break;
	 case 36: // 토큰 == 에 대한 처리를 수행하는 상태.
		 token.index = 2; strcpy(token.sub_kind, "EQ");	strcpy(token.str, "==");
		 strcpy(token.typ, "ROP");
		 return token;
	 case 37: // 토큰 > 에 대한 처리를 수행하는 상태.
		 ungetc(c, fp);
		 token.index = 8; strcpy(token.str, "=");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 38:
		 c = fgetc(fp);
		 if (c == '=') state = 39;
		 else state = 40;
		 break;
	 case 39:
		 token.index = 2; strcpy(token.sub_kind, "NE");	 strcpy(token.str, "!=");
		 strcpy(token.typ, "ROP");
		 return token;
	 case 40:
		 ungetc(c, fp);
		 token.index = 10;  strcpy(token.str, "!"); // NOT
		 strcpy(token.typ, "ROP");
		 return token;
	 case 45:
		 token.index = 16; 		 strcpy(token.str, "+=");
		 return token;
	 case 46:
		 ungetc(c, fp);
		 token.index = 3;  strcpy(token.str, "+");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 47:
		 token.index = 14;		 strcpy(token.str, "++");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 48:
		 token.index = 15;		 strcpy(token.str, "--");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 49:
		 ungetc(c, fp);
		 token.index = 17;  strcpy(token.str, "-=");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 50:
		 ungetc(c, fp);
		 token.index = 4;		 strcpy(token.str, "-");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 51:
		 token.index = 9;		 strcpy(token.str, "->");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 52:
		 token.index = 5;		 strcpy(token.str, "*");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 53:
		 token.index = 30;		 strcpy(token.str, "\\");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 54:
		 token.index = 7;		 strcpy(token.str, "%");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 55:
		 token.index = 11;		 strcpy(token.str, ".");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 56:
		 token.index = 12;		 strcpy(token.str, ",");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 57:
		 token.index = 18;		 strcpy(token.str, "(");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 58:
		 token.index = 19;		 strcpy(token.str, ")");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 59:
		 token.index = 20;		 strcpy(token.str, "{");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 60:
		 token.index = 21;		 strcpy(token.str, "}");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 61:
		 token.index = 22;		 strcpy(token.str, "[");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 62:
		 token.index = 23;		 strcpy(token.str, "]");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 63:
		 token.index = 24;		 strcpy(token.str, ":");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 64:
		 token.index = 25;		 strcpy(token.str, ";");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 65: // from double quote - string
	 	bp = 0;
		str_bp = 0;
		push_buf_one(buf, &bp, c);
		state = 77;
		break;
	 case 66: // from single quote - char
	 	bp = 0;
		push_buf_one(buf, &bp, c);
		state = 80;
		break;
	 case 67:
		 token.index = 28;		 strcpy(token.str, "#");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 68:
		 c = fgetc(fp);
		 if (c == '|') 	state = 69;
		 else 	 state = 70;
		 break;
	 case 69:
		 token.index = 2;	strcpy(token.sub_kind, "OR");	 strcpy(token.str, "||");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 70:
		 ungetc(c, fp);
		 token.index = 29;	 strcpy(token.str, "|");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 71:
		 token.index = EOF_TOK; strcpy(token.str, "EOF");
		 strcpy(token.typ, "EOF");
		 return token;
	 case 72:
		 ungetc(c, fp);
		 token.index = 6; strcpy(token.str, "/");
		 return token;
	 case 73:
		 token.index = 6; strcpy(token.str, "/");
		 strcpy(token.typ, "Symbol");
		 return token;
	 case 74:
		 c = fgetc(fp);
		 if (c == '\n' ) state = 0;
		 else if (c == EOF) state = 71;
		 else state = 74;
		 break;
	 case 75:
		 c = fgetc(fp);
		 if (c == '*') state = 76;
		 else if (c == EOF) state = 71;
		 else state = 75;
		 break;
	 case 76:
		 c = fgetc(fp);
		 if (c == '/') state = 0;
		 else if (c == EOF) state = 71;
		 else state = 75;
		 break;
	/* string state start */
	 case 77:
	 	c = fgetc(fp);
		if (c == EOF) state = 25;
		else if (c == '"') { push_buf_one(buf, &bp, c); state = 79; }
		else if (c == '\\') { push_buf_one(buf, &bp, c); state = 78; }
		else { push_buf_one(buf, &bp, c); push_buf_one(token.string_constant, &str_bp, c); }
		break;
	 case 78:
	 	c = fgetc(fp);
		if (c == EOF) state = 25;
		else if (c == '"') { push_buf_one(buf, &bp, c); push_buf_one(token.string_constant, &str_bp, c); state = 77; }
		else if (c == 't') { push_buf_one(buf, &bp, 9); push_buf_one(token.string_constant, &str_bp, 9); state = 77; }
		else if (c == 'n') { push_buf_one(buf, &bp, 10); push_buf_one(token.string_constant, &str_bp, 10); state = 77; }
		else { push_buf_one(buf, &bp, c); push_buf_one(token.string_constant, &str_bp, '\\'); push_buf_one(token.string_constant, &str_bp, c); }
		break;
	 case 79:
	 	token.index = 54;
		strcpy(token.str, buf);
		strcpy(token.typ, "String");
		return token;
	/* string state end */
	/* char state start */
	 case 80:
	 	c = fgetc(fp);
		if (c == '\\') { push_buf_one(buf, &bp, c); state = 82; }
		else { push_buf_one(buf, &bp, c); token.char_constant = c; state = 81; }
		break;
	 case 81:
	 	c = fgetc(fp);
		if (c == EOF) state = 25;
		else if (c == '\'') { push_buf_one(buf, &bp, c); state = 83; }
		else { push_buf_one(buf, &bp, c); state = 25; }
		break;
	 case 82:
		c = fgetc(fp);
	 	if (c == '0') { push_buf_one(buf, &bp, 0); token.char_constant = 0; state = 81; }
		else if (c == 't') { push_buf_one(buf, &bp, 9); token.char_constant = 9; state = 81; }
		else if (c == 'n') { push_buf_one(buf, &bp, 10); token.char_constant = 10; state = 81; }
		else { push_buf_one(buf, &bp, c); state = 25; }
	 	break;
	 case 83:
	 	token.index = 55;
		strcpy(token.str, buf);
		strcpy(token.typ, "Char");
		return token;
	/* char state end */
      default: printf ("Unrecognizable token! Stop generating tokens.\n" );
		  token.index = UNK; strcpy(token.str, "UNK");
		  return token;
     } // switch
   } //while
 }

 //---------print start--------------
 void print_token (tokentype a_tok, FILE *ofp) {
   fprintf(ofp, "%s\tToken_idx: %d,  %s ", a_tok.str, a_tok.index, a_tok.typ); // 토큰 종류 출력 (종류는 스트링으로 대체함)
   printf("%s\tToken_idx: %d,  %s ", a_tok.str, a_tok.index, a_tok.typ);

   if (a_tok.index == 1) { // this is number token.
	   if (strcmp(a_tok.sub_kind, "in") == 0) {
		   fprintf(ofp, "   integer  Val= %d", a_tok.sub_data); // 정수토큰
		   printf("   integer  Val= %d", a_tok.sub_data);
	   }
	   else if (strcmp(a_tok.sub_kind, "do") == 0) {
		   fprintf(ofp, "   double  Val= %10.7f", a_tok.rnum); // 실수 토큰
		   printf("   double  Val= %10.7f", a_tok.rnum);
	   }
   }
   else if (a_tok.index == 0) {
	   fprintf(ofp, "  Symtbl_idx = %5d", a_tok.sub_data); // id 의 심볼테이블 엔트리
	   printf("  Symtbl_idx = %5d", a_tok.sub_data);
   }
   else if (a_tok.index == 54) { // string 토큰
	   fprintf(ofp, "  String Var= %s", a_tok.string_constant);
	   printf("  String Var= %s", a_tok.string_constant);
   }
   else if (a_tok.index == 55) { // char 토큰
	   fprintf(ofp, "  Char Var= %c", a_tok.char_constant);
	   printf("  Char Var= %c", a_tok.char_constant);
   }
   else;
   fprintf(ofp, "\n" ) ;
   printf("\n");

   fflush(ofp);
 }
//---------print end-------------

int main(void) {
	tokentype a_tok;
	char source_file[30] = "sourcefile.txt";
	char output_file[30] = "outputfile.txt";
	char c;
	int i ;

	fp = fopen(source_file, "r"); // 입력 소스 파일
	if (!fp)  { printf("file open error of file = %s", source_file); return 1; }
	ofp = fopen(output_file, "w"); // 출력 파일
	if (!ofp)  { printf("file open error of file = %s", output_file); return 1; }

	while ( 1 ) {
		a_tok = lexan();
		if (a_tok.index == EOF_TOK) // EOF 토큰
			break; // there will be no more token. so exit.
		else if (a_tok.index == UNK)
			break ; // terminate token generation since wrong token has appeared.
		else
			print_token(a_tok, ofp);
	}
	fclose(fp);
	fclose(ofp);
	printf("Program ends with success.\n\n");
	return 0;
} // main
