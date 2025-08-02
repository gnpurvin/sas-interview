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
// simplicity.
// 6. that the size of the given query can be contained in size_t
// 7. that the query is in all lowercase. again, assumed for the sake of
// simplicity
// 8. assume that we're not in a situation where memory is very expensive
// 9. caller is responsible for freeing the returned string

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

// mapping of supported DBMSs to their substring function
static const char* k_functionMapping[DBMS_MAX] = {"substr", "substring",
                                                  "sbstr"};

// if current char is (, check if we've hit a match
static bool checkMatch(const char* pQuery, size_t queryIndex,
                       size_t substrIndex, Range* range, size_t numMatches,
                       bool isPossibleMatch[]) {
    // looking for the ( before we check if we've found a full match
    if (pQuery[queryIndex] != '(') {
        return false;
    }
    for (DbmsType possibleDbms = DBMS_1; possibleDbms < DBMS_MAX;
         possibleDbms++) {
        // if we've found all chars in one of the substring functions, and
        // the next char is a (, then we've hit a match
        if (isPossibleMatch[possibleDbms] &&
            substrIndex == strlen(k_functionMapping[possibleDbms])) {
            // match
            range->end = queryIndex - 1;  // -1 to account for the (
            range->begin = queryIndex - substrIndex;
            return true;  // match found
        }
    }
    return false;  // no match found
}

// compare the char at index pQuery[queryIndex] to each char from
// k_functionMapping at index substrIndex if the char is NOT a match, set
// isPossibleMatch to false
static bool isCurrentCharMatch(const char* pQuery, size_t queryIndex,
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

static char* rebuildQuery(char* pQuery, DbmsType dbms) {
    if (!pQuery) {
        printf("Null query provided");
        return NULL;
    }
    // if the string is too short for the shortest substring function to fit, it
    // definitely isn't in there
    if (strlen(pQuery) < strlen(k_functionMapping[DBMS_3])) {
        return pQuery;
    }
    if (dbms < DBMS_1 || dbms >= DBMS_MAX) {
        printf("Unsupported DBMS type provided");
        return NULL;
    }

    // a simpler way would be to use strstr to search for each substring
    // function. however, for really large strings, this would be inefficient,
    // so instead we'll try to look for all 3 simultaneously character by
    // character
    size_t substrIndex = 0;
    Range range = {0};
    size_t numMatches = 0;
    bool isPossibleMatch[DBMS_MAX] = {true, true, true};
    // if we need to conserve memory, our allocation strategy should be
    // revisited. for now, since it's a common strategy to just double the size
    // of your buffer when extending, we'll just go ahead and double it. this
    // should be sufficient since the worst case increase in space would be
    // replacing sbstr -> substring, which is replacing 5 chars with 9
    // if additional DBMSs with different substring functions are supported,
    // this would need to be revisited
    // using calloc so we aren't cat-ing to uninitialized memory in the buffer
    char* pBuffer = (char*)calloc((strlen(pQuery) * 2) + 1, sizeof(char));
    char* pQueryIt = pQuery;
    size_t firstUncheckedCharIndex = 0;
    for (size_t queryIndex = 0; queryIndex < strlen(pQuery); queryIndex++) {
        // if we hit a match, save the range to our list
        if (checkMatch(pQuery, queryIndex, substrIndex, &range, numMatches,
                       isPossibleMatch)) {
            substrIndex = 0;
            // cat the string from before our match to the buffer
            size_t beforeSubstringLen = (range.begin) - firstUncheckedCharIndex;
            // strncat adds null terminator for us
            strncat(pBuffer, pQueryIt, beforeSubstringLen);
            // cat the proper substring function to the buffer
            strncat(pBuffer, k_functionMapping[dbms],
                    strlen(k_functionMapping[dbms]));
            // move pQueryIt forward past our match
            pQueryIt += (range.end - firstUncheckedCharIndex) + 1;
            // +1 so we aren't incluiding range.end
            firstUncheckedCharIndex = range.end + 1;
            // reset our range
            range.begin = 0;
            range.end = 0;
            numMatches++;
            // reset array of matches
            memset(isPossibleMatch, true,
                   sizeof(isPossibleMatch) / sizeof(bool));

            continue;
        }
        // parsing through char by char
        if (isCurrentCharMatch(pQuery, queryIndex, substrIndex,
                               isPossibleMatch)) {
            substrIndex++;
        } else {
            substrIndex = 0;
            // reset array of matches
            memset(isPossibleMatch, true,
                   sizeof(isPossibleMatch) / sizeof(bool));
        }
    }

    // add the rest of the query to our string
    strncat(pBuffer, pQueryIt, strlen(pQueryIt));
    return pBuffer;
}

void testQuery(char* pTestQuery, DbmsType dbms) {
    char* pResultQuery = rebuildQuery(pTestQuery, dbms);
    printf("original query: %s\n", pTestQuery);
    printf("Fixed query:    %s\n", pResultQuery);
    free(pResultQuery);
}

int main() {
    for (DbmsType possibleDbms = DBMS_1; possibleDbms < DBMS_MAX;
         possibleDbms++) {
        printf("\nTesting DBMS arg %u\n", possibleDbms);
        // basic test cases
        // 1. simple replacement of one substring function
        printf("\n1. simple replacement of one substring function\n");
        testQuery("aasubstring(aa)aa", possibleDbms);
        testQuery("aasubstr(aa)aa", possibleDbms);
        testQuery("aasbstr(aa)aa", possibleDbms);

        // 2. multiple matches for same substring function
        printf("\n2. multiple matches for same substring function\n");
        testQuery("aasubstring(aa)aasubstring(aa)", possibleDbms);
        testQuery("aasubstr(aa)aasubstr(aa)", possibleDbms);
        testQuery("aasbstr(aa)aasbstr(aa)", possibleDbms);

        // // 3. multiple matches for differing substring function
        printf("\n3. multiple matches for differing substring function\n");
        testQuery("aasubstring(aa)aasubstr(aa)sbstr(aa)", possibleDbms);
        testQuery("aasubstr(aa)aasbstr(aa)substring(aa)", possibleDbms);
        testQuery("aasbstr(aa)aasubstring(aa)substr(aa)", possibleDbms);

        // negative tests
        printf("\nNegative tests\n");
        // 1. mixing 2 substring functions
        printf("1. mixing 2 substring functions\n");
        testQuery("aasbbstring(aa)aa", possibleDbms);
        // 2. 1 char off
        printf("2. 1 char off\n");
        testQuery("aasubstrin(aa)aa", possibleDbms);
        testQuery("aasubst(aa)aa", possibleDbms);
        testQuery("aasbst(aa)aa", possibleDbms);
        testQuery("aaubstring(aa)aa", possibleDbms);
        testQuery("aaubstr(aa)aa", possibleDbms);
        testQuery("aabstr(aa)aa", possibleDbms);
        // 3. missing (
        printf("3. missing (\n");
        testQuery("aasubstringaa)aa", possibleDbms);
        testQuery("aasubstraa)aa", possibleDbms);
        testQuery("aasbstraa)aa", possibleDbms);
    }

    return 0;
}