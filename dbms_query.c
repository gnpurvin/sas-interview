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
                Range* range, size_t numMatches) {
    // looking for the ( before we check if we've found a full match
    if (pQuery[queryIndex + 1] != '(') {
        return false;
    }
    for (DbmsType possibleDbms = DBMS_1; possibleDbms < DBMS_MAX;
         possibleDbms++) {
        // if we've found all chars in one of the substring functions, and
        // the next char is a (, then we've hit a match
        if (substrIndex == strlen(k_functionMapping[possibleDbms]) - 1) {
            // match
            range->begin = queryIndex - substrIndex;
            range->end = queryIndex;
            substrIndex = 0;  // reset for next match
            printf("match found. begin: %lu end %lu\n", range->begin,
                   range->end);
            return true;  // match found
        }
    }
    return false;  // no match found
}

bool isCurrentCharMatch(const char* pQuery, size_t queryIndex,
                        size_t substrIndex, bool isPossibleMatch[]) {
    bool isMatch = false;
    for (DbmsType possibleDbms = DBMS_1; possibleDbms < DBMS_MAX;
         possibleDbms++) {
        // if we've already decided we don't have a match for this DBMS's
        // substring function, no need to check
        if (!isPossibleMatch[possibleDbms]) {
            continue;
        }
        // if chars match, this DBMS may be a match
        if (pQuery[queryIndex] ==
            k_functionMapping[possibleDbms][substrIndex]) {
            isMatch = true;
        } else if (substrIndex > 0) {
            // if substrIndex is 0, we haven't found any matching characters yet
            // don't mark as not a match until we've found our first matching
            // char
            isPossibleMatch[possibleDbms] = false;
        } else {
            // no-op
        }
    }
    return isMatch;
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
    Range range = {0};
    size_t numMatches = 0;
    size_t bufferSize = strlen(pQuery) + 1;
    bool isPossibleMatch[DBMS_MAX] = {true, true, true};
    for (size_t queryIndex = 0; queryIndex < stopSearch; queryIndex++) {
        // if we hit a match, save the range to our list
        if (checkMatch(pQuery, queryIndex, substrIndex, &range, numMatches)) {
            substrIndex = 0;
            numMatches++;
            // update buffer size to size of the matched size and add size of
            // the replacement string
            bufferSize -= (range.end - range.end);
            bufferSize += strlen(k_functionMapping[dbms]);
            range.begin = 0;
            range.end = 0;
            continue;
        }
        // parsing through char by char
        if (isCurrentCharMatch(pQuery, queryIndex, substrIndex,
                               isPossibleMatch)) {
            substrIndex++;
        } else {
            substrIndex = 0;
        }
    }

    // TODO: calculate size of the new query and allocate it
    printf("numMatches = %lu\n", numMatches);
    printf("bufferSize = %lu\n", bufferSize);

    // TODO: use memcpy to copy from query until beginning of range
    // then copy k_functionMapping[dbms]
    // then from end of range to beginning of the next range
    // then, replace with the appropriate substring command for the given DBMS
}

int main() {
    // basic test cases
    rebuildQuery("aasubstring(aa)aa", DBMS_1);
    rebuildQuery("aasubstr(aa)aa", DBMS_1);
    rebuildQuery("aasbstr(aa)aa", DBMS_1);

    // edge cases
    // 1. mixing 2 substring functions
    rebuildQuery("aasbbstring(aa)aa", DBMS_1);
    return 0;
}