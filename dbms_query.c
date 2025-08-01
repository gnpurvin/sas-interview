// assumptions

// 1. DBMS -> substring function mapping are known before hand
// 2. provided query string is null-terminated. if it's not, we can add a
// parameter for the size of the string
// 3. input is sanitized before we receive it. i.e. SQL injection isn't a
// concern. if it's not, we will need to do that ourselves for the sake of
// security
// 4. that the given query does not contain any of the substring functions as an
// escaped string. this assumption is for the sake of simplicity.
// 5. that the given query is valid SQL. again, this is for the sake of
// simplicity
// 6. that the size of the given query can be contained in size_t

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    DBMS_1,
    DBMS_2,
    DBMS_3,
    // update if new a DBMS is supported
    DBMS_MAX
} DbmsType;

typedef struct {
    size_t begin;
    size_t end;
} Range;

const char* k_functionMapping[DBMS_MAX] = {"substr", "substring", "sbstr"};

bool checkMatch(const char* pQuery, size_t queryIndex, size_t substrIndex,
                Range* ranges, size_t numMatches) {
    for (DbmsType possibleDbms = DBMS_1; possibleDbms < DBMS_MAX;
         possibleDbms++) {
        // if we've found all chars in one of the substring functions, and
        // the next char is a (, then we've hit a match
        if (substrIndex == strlen(k_functionMapping[possibleDbms]) - 1 &&
            pQuery[queryIndex + 1] == '(') {
            // match
            printf("match found\n");
            ranges[numMatches].begin = queryIndex - substrIndex;
            ranges[numMatches].end = queryIndex;
            substrIndex = 0;  // reset for next match
            return true;      // match found
        }
    }
    return false;  // no match found
}

void rebuildQuery(char* pQuery, DbmsType dbms) {
    if (!pQuery) {
        printf("Null query provided");
        return;
    }
    // if the string is too short for the shortest substring function to fit, it
    // definitely isn't in there
    if (strlen(pQuery) < strlen(k_functionMapping[DBMS_3])) {
        printf("Empty query provided");
        return;
    }
    if (dbms < DBMS_1 || dbms >= DBMS_MAX) {
        printf("Unsupported DBMS type provided");
        return;
    }

    // a simpler way would be to use strstr to search for each substring
    // function. however for really large strings, this would be inefficient, so
    // instead we'll try to look for all 3 simultaneously character by character
    size_t substrIndex = 0;
    // stop searching 1 before end of query, because we want to look ahead 1
    // char
    size_t stopSearch = strlen(pQuery) - 1;
    Range ranges[10] = {0};  // TODO: just doing this for sake of simplicity.
                             // really should be a dynamic array
    size_t numMatches = 0;
    for (size_t queryIndex = 0; queryIndex < stopSearch; queryIndex++) {
        // if we hit a match, save the range to our list
        if (checkMatch(pQuery, queryIndex, substrIndex, ranges, numMatches)) {
            substrIndex = 0;
            numMatches++;
            continue;
        }
        // parsing through char by char
        if (pQuery[queryIndex] == k_functionMapping[DBMS_2][substrIndex]) {
            // printf("substring case, substrIndex = %lu, queryIndex = %lu\n",
            //        substrIndex, queryIndex);
            substrIndex++;
        } else if (pQuery[queryIndex] ==
                   k_functionMapping[DBMS_1][substrIndex]) {
            // substr case
            // printf("substr case, substrIndex = %lu, queryIndex = %lu\n",
            //        substrIndex, queryIndex);
            substrIndex++;
        } else if (pQuery[queryIndex] ==
                   k_functionMapping[DBMS_3][substrIndex]) {
            // sbstr case
            substrIndex++;
        } else {
            substrIndex = 0;
        }
    }

    // TODO: calculate size of the new query and allocate it
    printf("numMatches = %lu\n", numMatches);
    // then, replace with the appropriate substring command for the given DBMS
    for (size_t i = 0; i < numMatches; i++) {
        printf("match: begin = %lu, end = %lu\n", ranges[i].begin,
               ranges[i].end);
        // TODO: use memcpy to copy from query until beginning of range
        // then copy k_functionMapping[dbms]
        // then from end of range to beginning of the next range
    }
}

int main() {
    printf("hello world\n");

    rebuildQuery("aasubstring(aa)aa", DBMS_1);
    rebuildQuery("aasubstr(aa)aa", DBMS_1);
    rebuildQuery("aasbstr(aa)aa", DBMS_1);
    return 0;
}