// Beatrise Ivanovska 251RDC088
// https://github.com/bech27/Lab1_calc_c
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
/*Resources used:
1. https://stackoverflow.com/questions/14901919/when-can-i-omit-curly-braces-in-c (used to do a bit less work and not confuse myself in if else (etc)statements with those braces)
2. https://claude.ai/share/d80ae454-297d-4528-8988-120bc46dc452
3. Lecture presentations 3 to 6 (literally throughout the whole thing)
4. https://stackoverflow.com/questions/2575048/arrow-operator-usage-in-c the arrow pointers for a pointer rather than reference
5.https://cburch.com/books/cptr/ for understanding how pointers work better
6. https://cppreference.com/c/language/operator_precedence explanation of each pointer for clarification
7. https://www.geeksforgeeks.org/c/isdigit-function-in-c-cpp-with-examples/ to check if for parse_primary  those are digits
9. https://www.geeksforgeeks.org/c/unsigned-char-in-c-with-examples/ what is proper syntax for using usnsigned char
9. https://stackoverflow.com/questions/1799337/prior-declarations-of-functions to understand why it threw error and why i should declare before
10. https://www.geeksforgeeks.org/c/snprintf-c-library/ how to store in array buffer
11. https://www.geeksforgeeks.org/c/how-to-use-typedef-for-struct-in-c/ for the simplified struct definition, in what case to use it
12. https://stackoverflow.com/questions/39002052/how-can-i-print-to-standard-error-in-c-with-printf how to use stderr in correct syntax
13. https://www.geeksforgeeks.org/cpp/fseek-in-c-with-example/ how to move the pointet to the end of the file*/ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define NAME "beatrise" //6th presentation slide 28
#define LASTNAME  "ivanovska"
#define ID "251RDC088"

/*I used State and pointer p to go through the file. To print the error,
it must know how many places we moved and therefore also keep the start position. So in this case
the position is how much p moved from the beginning of the file +1. I didnt use a seperate tokenizer struct because i
used s->p as a "cursor or shifter" to go through char by char. */
typedef struct {
    const char *start; //begins file buffer + 4th presentatin slide 18 + cburch on pointers animation
    const char *p; //shows current reading position
    int err; //1 when first error is found
    size_t errpos;//position of error
} State; //6th presentation slide 20, simplified struct geeksforgeeks


/*Because position is one based, i added +1 because p in itself that counts the newlines, spaces etc
is 0 based originally. As for error if there is no newline, +1 is added to the length filew to indicate the error position.
Thats what function here() in this work does.*/
static size_t here(const State *s) {
    return (size_t)(s->p - s->start) + 1;
}


/*Fail stores the first error that occured to not accidentally overwrite it with the next one if there is such.
So it holds Error: <curr. pos.>*/
static void fail(State *s, size_t pos){
    if(!s->err){
        s->err = 1;
        s->errpos =pos;
    }
}


/*i used only skip spaces and tabs (common in the functions between operatores and numbers)and NOT newline because newline ends
and expression and if the newline were to be ignored, i could accidentally put two expressions into one.*/
static void skip_spaces(State *s){
    while(*s->p == ' ' || *s->p == '\t') s->p++; //stackoverflow, omit braces + 3rd presentation slide 18
}


/*i declared it in advamnce so compiler doesnt yell error because parse_primary and parse_expr call each other*/
static long long parse_expr(State *s); //prior declaration stackoverflow


/*i did it as said in lab about the expression 'importance' level. Here +/- is weakest, then * and division and only then ** and
digits are most important). So it works like this: eg. 3 + 2 * 5, so pars_expr starts, asks parse_term for left thing and gets 3
and then sees +, which is the weakest. So the parse_term asks the right thing and gets 2 * 5, which is stronger so it gets compute dfirst.
only then goes 3 + 10. * is a deeper function than + or -, so it goes first

** is right-associative because in an example of 2 ** 3 ** 2 power is from both sides, so parse_power calls itself again for the exponent
and marks the expression correct as 2**(3**2)=512 and not (2**3)**2=64. left to right loop would do the incorrect version, not the correct one.

error positions get triggered when in the place where a number or a paranthesee should be, there isnt. Like the example of error:
-5 + 3, first, function expects a number, not an operator, thats why it will report its current position (in this case 1) as error.

'(' calls parse_expr and parse_primary because in parantheses there could be a full expression*/
static long long parse_primary(State  *s){
    skip_spaces(s);
    if(s->err) return 0; //3rd presentation slide 6
    if(*s->p == '('){
        s->p++;
        long long v = parse_expr(s);
        skip_spaces(s);
        if(*s->p != ')'){
            fail(s, here(s));
            return 0;
        }
        s->p++;
        return v;
    }
    if(isdigit((unsigned char)*s->p)){ //geeksforgeeks to understand correct syntax how to use
        long long v = 0;
        while(isdigit((unsigned char)*s->p)){
            v=v * 10+ (*s->p - '0');
            s->p++;
        }
        return v;
    }
    fail(s, here(s));
    return 0;
}


/*For-loop isnt used here because recursion makes it right-associative (as needed). Negative exponent used an exp_pos
because e.g. 2**(1-2) would thro an error at parenthesis, which begins the exponent. So i use here() to save wheever it
is before reading the exponent and only then fail(exp_pos) if exp<0.*/
static long long parse_power(State *s){
    long long base = parse_primary(s);
    if(s->err) return 0;

    skip_spaces(s);
    if(s->p[0] == '*' && s->p[1] == '*'){
        s->p += 2;
        skip_spaces(s);

        size_t exp_pos = here(s);
        long long exp = parse_power(s);
        if(s->err) return 0;

        if(exp<0){
            fail(s, exp_pos);
            return 0;
        }
        long long result = 1;
        for(long long i = 0; i<exp; i++) result *=base;
        return result;
    }
    return base;
}


/* Here is * /. they are in parse_term and not in parse_exp because they must bind tighther than + or -. Here i use a for loop
which is left-associative, reading left to right so 100/7/2 looks like (100/7)/2.

break is inserted so ** isnt accidentally mistaken for *, which should not be treated the same.

i used right_pos for division with a zero because if it were from left and an example like 10 / 0 were to appear, an error would be thrown.
So it is save here() and after that right side is being read.*/
static long long parse_term(State *s){
    long long left = parse_power(s);
    if(s->err) return 0;
    for(;;){ //3rd presentation from lectures slide 17
        skip_spaces(s);
        char op = *s->p; /*cburch and arrow pointers to understand what each does
        to go to pointer from struct->go where pointer points->read the pointed value*/
        if(op =='*' && s->p[1] == '*') break;
        if(op != '*' && op != '/') break;

        s->p++;
        skip_spaces(s);

        size_t right_pos = here(s);
        long long right = parse_power(s);
        if(s->err) return 0;

        if(op == '*') {
            left *=right;
        } else {
            if(right == 0){
                fail (s,right_pos);
                return 0;
            }
            left /= right;
        }
    }
    return left;
}


/*Here is the body of the function i declared earlier. Now all the important other functions exist so they can be called. 
+ - are weakest operators here so they just combine the results thyat were gotten before.*/
static long long parse_expr(State *s){
    long long left = parse_term(s);
    if(s->err) return 0;

    for(;;){
        skip_spaces(s);
        char op = *s->p;
        if (op != '+'&& op  != '-') break;

        s->p++;
        skip_spaces(s);
        long long right = parse_term(s);
        if(s->err) return 0;
        if(op=='+') left += right; //here and one below stackoverflow, omit braces
        else left -= right;
    }
    return left;
}


/*It checks that there is no stray character or symbol or etc. If there is, it will call error because only end of line and
end of file are acceptable here. Anything that may sabotage the  correct expression is caught here. Also including an extra ')'*/
static void expect_line_end(State *s){
    skip_spaces(s);
    if(*s->p != '\0' && *s->p != '\n') fail(s, here(s));
}


/*It skips over the lines that go after # (unless it is in the middle of an expression (then it is an error)) because it is a
comment and carries no real sense. i do not delete it fro mthe buffer*/
static void skip_blank_comment(State *s){
    for(;;){
        skip_spaces(s);
        if (*s->p == '\0') return;
        if (*s->p == '\n') { s->p++; continue;}
        if (*s->p == '#') {
            while (*s->p && *s->p != '\n') s->p++;
            if(*s->p == '\n') s->p++;
            continue;
        }
    return;
    }
}


/*Every value here is long long and dot is invalid. i didnt succeed in making a solutions for task 10.

As for how i handle the largewr files without slowing down:
main reads file once into vuffer
only pointer s.p moves through buffer
each line is not copied into a new string to parse
output buffer grows by douybling cap *=2 which is why realloc doesnt happen often*/
static char *evaluate_file(const char *src){
    State s = { src, src, 0 ,0};
    size_t cap = 128, len = 0;
    char *out = malloc(cap);
    if(!out) return NULL;
    out[0] = '\0';

    while(*s.p){ //6th presentation slide 16
        skip_blank_comment(&s);//presentation 4 slide 18
        if(*s.p == '\0') break;
        long long value = parse_expr(&s);
        expect_line_end(&s);

        if(s.err){
            free(out); //6yh presentation slide 12
            char *msg = malloc(64); //6th presentation, slide 13
            if(!msg) return NULL;
            snprintf(msg, 64, "ERROR:%zu\n", s.errpos);//geeks for geeks on how to save on the buffer
            return msg;
        }
        char line[64];
        int n= snprintf(line, sizeof line, "%lld\n", value);
        if (len + (size_t)n + 1 > cap){
            cap *=2; //makes less copies when answer sheet grows bigger
            char *tmp = realloc(out, cap);
            if(!tmp) { free(out); return NULL;}
            out = tmp;
        }
        memcpy(out + len, line, (size_t)n);
        len += (size_t)n;
        out[len] = '\0';
        if(*s.p == '\n') s.p++;
    }
    return out;
}

int main(int argc, char **argv){
    const char *outdir = NULL, *input = NULL;


    /*created for mark 4 calc -o outdir input .txt*/
    for(int i =1; i<argc; i++){
        if(!strcmp(argv[i], "-o") && i + 1 < argc) outdir = argv[++i]; //5th presentation slide 25,27
        else if(argv[i][0] != '-') input = argv[i];
        else { fprintf(stderr, "unknown option\n"); return 1;} //correct syntax for stderr
    }
    if(!outdir || !input){
        fprintf(stderr, "usage: calc -o OUTDIR input.txt\n");
        return 1;
    }

    /*reads the whole file and the pointer walks through to avoid copying pieces of files*/
    FILE *fin = fopen(input, "rb"); //5th presentation slide 2, 4
    if(!fin) { perror(input); return 1;}
    fseek(fin, 0, SEEK_END); //how to get the file pointer to go to the end
    long n = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    char *src = malloc((size_t)n +  1);
    if(!src) { fclose(fin); return 1;}
    if(fread(src, 1, (size_t)n, fin) != (size_t)n){
        fprintf(stderr, "failed to read the file\n");
        free(src);
        fclose(fin);
        return 1;
    }
    src[n] = '\0';
    fclose(fin);

    char *result = evaluate_file(src);
    free(src);
    if(!result) return 1;

    const char *base = input;
    for (const char *q = input; *q; q++)
        if(*q == '/' || *q == '\\') base = q + 1;
    char stem[256], outpath[512]; //5th presentation slide 38
    snprintf(stem, sizeof stem, "%s", base); //geeks for geeks on how to save on the buffer
    char *dot = strstr(stem, ".txt");
    if(dot) *dot = '\0';
    snprintf(outpath, sizeof outpath, "%s/%s_%s_%s_%s.txt", outdir, stem, NAME, LASTNAME, ID); //geeks for geeks on how to save on the buffer

    mkdir(outdir, 0755); //claude on 0 passed and 0 failed, i couldnt really understand why it didnt pass/fail and what could be the reason (also when failed was 44)

    FILE *fout = fopen(outpath, "w"); //5th presentation slide 2, 4
    if(!fout){ perror(outpath); free(result); return 1;}
    fputs(result, fout);
    fclose(fout);
    free(result);
    return 0;
}
