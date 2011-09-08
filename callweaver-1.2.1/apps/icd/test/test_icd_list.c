#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include "CuTest.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_config.h"

/*-------------------------------------------------------------------------*
 * Helper functions
 *-------------------------------------------------------------------------*/

static icd_config_registry *registry = NULL;

static void CuStringDestroy(CuTest *tc, CuString *str);

static icd_config *get_config(CuTest *tc, char *size, char *name) {
    icd_config *config;
    icd_status result;

    config = create_icd_config(registry, "Test Config");
    CuAssertPtrNotNull(tc, config);
    result = icd_config__set_value(config, "size", size);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    result = icd_config__set_value(config, "name", name);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    return config;
}

static CuString *get_payload(CuTest* tc, char *str) {
    CuString *retval;

    CuAssertPtrNotNull(tc, str);
    retval = CuStringNew();
    CuAssertPtrNotNull(tc, retval);
    CuStringAppend(retval, str);
    return retval;
}

static void destroy_payload(CuTest *tc, CuString *str) {
    CuStringDestroy(tc, str);
}

static CuString **get_many_payloads(CuTest* tc, int number, int start, int step) {
    CuString **retval;
    CuString *str;
    int x;

    retval = (CuString **) malloc(sizeof(CuString *) * number);
    CuAssertPtrNotNull(tc, retval);

    for (x = 0; x < number; x++) {
        str = CuStringNew();
        CuAssertPtrNotNull(tc, str);
        CuStringAppendFormat(str, "%d", (x * step) + start);
        retval[x] = str;
    }
    return retval;
}

static void destroy_many_payloads(CuTest *tc, CuString **payloads, int size) {
    int x;

    CuAssertPtrNotNull(tc, payloads);
    for (x = 0; x < size; x++) {
        CuAssertPtrNotNull(tc, payloads[x]);
        CuStringDestroy(tc, payloads[x]);
        payloads[x] = NULL;
    }
}

typedef struct {
    int id;
    CuString **nodeset;
    CuTest *tc;
    sem_t *init_threads;
    sem_t *finished_threads;
    int nodecount;
    int threadcount;
    icd_list *list;
} thread_param;

static thread_param *get_thread_param(CuTest* tc, int id,
        CuString **nodeset, int nodecount, int threadcount,
        icd_list *list, sem_t *init_threads, sem_t *finished_threads) {
    thread_param *retval;

    CuAssertPtrNotNull(tc, nodeset);
    CuAssertPtrNotNull(tc, *nodeset);
    CuAssertTrue(tc, nodecount > 0);
    CuAssertPtrNotNull(tc, nodeset[nodecount - 1]);
    CuAssertTrue(tc, threadcount > 0);
    CuAssertPtrNotNull(tc, list);
    CuAssertPtrNotNull(tc, init_threads);
    CuAssertPtrNotNull(tc, finished_threads);
    retval = (thread_param *) malloc(sizeof(thread_param));
    CuAssertPtrNotNull(tc, retval);
    retval->tc = tc;
    retval->id = id;
    retval->nodecount = nodecount;
    retval->threadcount = threadcount;
    retval->nodeset = nodeset;
    retval->list = list;
    retval->init_threads = init_threads;
    retval->finished_threads = finished_threads;
    return retval;
}

static void destroy_thread_param(CuTest* tc, thread_param *param) {
    CuAssertPtrNotNull(tc, param);
    free(param);
}

/* Money method for threads */
static void *thread_fn(void *ptr) {
    thread_param *param;
    icd_status result;
    int init_threadcount;
    int x;

    param  = (thread_param *)ptr;

    /* These have to be regular assertions, as we have no test case if either are false */
    assert (ptr != NULL);
    assert (param->tc != NULL);

    CuAssertPtrNotNull(param->tc, param->nodeset);
    CuAssertPtrNotNull(param->tc, param->nodeset[0]);
    CuAssertTrue(param->tc, param->nodecount > 0);
    CuAssertPtrNotNull(param->tc, param->nodeset[param->nodecount - 1]);
    CuAssertTrue(param->tc, param->threadcount > 0);
    CuAssertPtrNotNull(param->tc, param->list);
    CuAssertPtrNotNull(param->tc, param->finished_threads);

    /* Wait on all threads being set up before running */
    sem_post(param->init_threads);
    sem_getvalue(param->init_threads, &init_threadcount);
    while (init_threadcount < param->threadcount) {
        sched_yield();
        sem_getvalue(param->init_threads, &init_threadcount);
    }

    for (x = 0; x < param->nodecount; x++) {
        result = icd_list__push(param->list, param->nodeset[x]);
        /* printf("In thread %d, pushed node %s\n", param->id, param->nodeset[x]->buffer); */
        CuAssertTrue(param->tc, result == ICD_SUCCESS);
        sched_yield();
    }
    sem_post(param->finished_threads);
    return NULL;
}


static void *iterating_thread_fn(void *ptr) {
    thread_param *param;
    icd_status result;
    int init_threadcount=0;
    int finished_threadcount=0;
    int x;
    icd_list_iterator *iter;

    CuString *iter_payload =NULL;
    param  = (thread_param *)ptr;
    iter_payload = param->nodeset[0];

    /* These have to be regular assertions, as we have no test case if either are false */
    assert (ptr != NULL);
    assert (param->tc != NULL);

    CuAssertPtrNotNull(param->tc, param->nodeset);
    CuAssertPtrNotNull(param->tc, param->nodeset[0]);
    CuAssertTrue(param->tc, param->nodecount > 0);
    CuAssertPtrNotNull(param->tc, param->nodeset[param->nodecount - 1]);
    CuAssertTrue(param->tc, param->threadcount > 0);
    CuAssertPtrNotNull(param->tc, param->list);
    CuAssertPtrNotNull(param->tc, param->finished_threads);

    /* Wait on all threads being set up before running */
    sem_post(param->init_threads);
    sem_getvalue(param->init_threads, &init_threadcount);
    while (init_threadcount < param->threadcount) {
        sched_yield();
        sem_getvalue(param->init_threads, &init_threadcount);
    }

    while (finished_threadcount == 0) {
        sem_getvalue(param->finished_threads, &finished_threadcount);
        iter = icd_list__get_iterator(param->list);
        while (icd_list_iterator__has_more(iter)) {
            iter_payload = (CuString *)icd_list_iterator__next(iter);
            if (iter_payload !=NULL)
                printf("AF iter_payload[%s]  \n",iter_payload->buffer);            
        }
        sched_yield();
   	    destroy_icd_list_iterator(&iter); 
    }
    sem_post(param->finished_threads);
printf("AF *iterating_thread_fn init_th[%d], finished_th[%d]  \n",init_threadcount, finished_threadcount);
    return NULL;
}


static void *removing_thread_fn(void *ptr) {
    thread_param *param;
    icd_status result;
    int init_threadcount;
    int x;
    CuString *popped_payload;

    param  = (thread_param *)ptr;

    /* These have to be regular assertions, as we have no test case if either are false */
    assert (ptr != NULL);
    assert (param->tc != NULL);

    CuAssertPtrNotNull(param->tc, param->nodeset);
    CuAssertPtrNotNull(param->tc, param->nodeset[0]);
    CuAssertTrue(param->tc, param->nodecount > 0);
    CuAssertPtrNotNull(param->tc, param->nodeset[param->nodecount - 1]);
    CuAssertTrue(param->tc, param->threadcount > 0);
    CuAssertPtrNotNull(param->tc, param->list);
    CuAssertPtrNotNull(param->tc, param->finished_threads);

    /* Wait on all threads being set up before running */
    sem_post(param->init_threads);
    sem_getvalue(param->init_threads, &init_threadcount);
    while (init_threadcount < param->threadcount) {
        sched_yield();
        sem_getvalue(param->init_threads, &init_threadcount);
    }

    popped_payload = (CuString *)icd_list__peek(param->list);
    while (popped_payload != NULL) {
        result = icd_list__remove_by_element(param->list, popped_payload);
        CuAssertTrue(param->tc, result == ICD_SUCCESS);
        popped_payload = (CuString *)icd_list__peek(param->list);
        sched_yield();
    }
printf("Destroy List removing_thread_fn \n");
    destroy_icd_caller_list(&(param->list));
    sem_post(param->finished_threads);
printf("AF done removing_thread_fn  \n");
    return NULL;
}




static int cmp_string_order(void *arg1, void *arg2) {
    CuString *str1;
    CuString *str2;

    str1 = (CuString *)arg1;
    str2 = (CuString *)arg2;

    return strcmp(str1->buffer, str2->buffer);
}

static int cmp_int_order(void *arg1, void *arg2) {
    CuString *str1;
    CuString *str2;
    int val1;
    int val2;

    str1 = (CuString *)arg1;
    str2 = (CuString *)arg2;
    val1 = atoi(str1->buffer);
    val2 = atoi(str2->buffer);

    if (val1 > val2) {
        return 1;
    } else if (val1 == val2) {
        return 0;
    }
    return -1;
}

static void CuStringDestroy(CuTest *tc, CuString *str) {
    CuAssertPtrNotNull(tc, str);
    if (str->buffer != NULL) {
        free(str->buffer);
    }
    free(str);
}


/*-------------------------------------------------------------------------*
 * icd_list Tests
 *-------------------------------------------------------------------------*/

/*
 * This just creates a list, ensures it is ok, and deletes it.
 */
void test_icd_list__create(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    char *list_name = "test.create";

    config = get_config(tc, "10", list_name);
    CuAssertPtrNotNull(tc, config);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);

    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));
    CuAssertStrEquals(tc, list_name, icd_list__get_name(list));
    result = destroy_icd_list(&list);
    CuAssertPtrEquals(tc, NULL, list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Creates a list, pushes an entry on, pops it back off, and deletes the list.
 */
void test_icd_list__pushpop(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    CuString *payload;
    CuString *popped_payload;
    char *str = "1";
    char *list_name = "test.pushpop";

    payload = get_payload(tc, str);
    config = get_config(tc, "10", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);

    result = icd_list__push(list, payload);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, payload, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    CuAssertStrEquals(tc, list_name, icd_list__get_name(list));

    destroy_payload(tc, payload);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test the FIFO default behaviour.
 */
void test_icd_list__defaultfifo(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    CuString *node1;
    CuString *node2;
    CuString *node3;
    CuString *node4;
    CuString *node5;
    CuString *popped_payload;
    char *str1 = "1";
    char *str2 = "2";
    char *str3 = "3";
    char *str4 = "4";
    char *str5 = "5";
    char *list_name = "test.defaultfifo";

    node1 = get_payload(tc, str1);
    node2 = get_payload(tc, str2);
    node3 = get_payload(tc, str3);
    node4 = get_payload(tc, str4);
    node5 = get_payload(tc, str5);
    config = get_config(tc, "10", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);

    result = icd_list__push(list, node1);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    result = icd_list__push(list, node2);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    result = icd_list__push(list, node3);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node1, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));

    result = icd_list__push(list, node4);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node2, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node3, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));

    result = icd_list__push(list, node5);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node4, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node5, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrEquals(tc, NULL, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    CuAssertStrEquals(tc, list_name, icd_list__get_name(list));

    destroy_payload(tc, node5);
    destroy_payload(tc, node4);
    destroy_payload(tc, node3);
    destroy_payload(tc, node2);
    destroy_payload(tc, node1);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test the FIFO default behaviour.
 */
void test_icd_list__fifo(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    CuString *node1;
    CuString *node2;
    CuString *node3;
    CuString *node4;
    CuString *node5;
    CuString *popped_payload;
    char *str1 = "1";
    char *str2 = "2";
    char *str3 = "3";
    char *str4 = "4";
    char *str5 = "5";
    char *list_name = "test.fifo";

    node1 = get_payload(tc, str1);
    node2 = get_payload(tc, str2);
    node3 = get_payload(tc, str3);
    node4 = get_payload(tc, str4);
    node5 = get_payload(tc, str5);
    config = get_config(tc, "10", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);
    result = icd_list__set_node_insert_func(list, icd_list__insert_fifo, NULL);
    CuAssertTrue(tc, result == ICD_SUCCESS);

    result = icd_list__push(list, node1);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    result = icd_list__push(list, node2);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    result = icd_list__push(list, node3);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node1, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));

    result = icd_list__push(list, node4);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node2, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node3, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));

    result = icd_list__push(list, node5);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node4, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node5, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrEquals(tc, NULL, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    CuAssertStrEquals(tc, list_name, icd_list__get_name(list));

    destroy_payload(tc, node5);
    destroy_payload(tc, node4);
    destroy_payload(tc, node3);
    destroy_payload(tc, node2);
    destroy_payload(tc, node1);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test the iterator.
 */
void test_icd_list__iterator(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    icd_list_iterator *iter;
    CuString *node1;
    CuString *node2;
    CuString *node3;
    CuString *node4;
    CuString *node5;
    CuString *popped_payload;
    char *str1 = "1";
    char *str2 = "2";
    char *str3 = "3";
    char *str4 = "4";
    char *str5 = "5";
    char *list_name = "test.iterator";
    int x;

    node1 = get_payload(tc, str1);
    node2 = get_payload(tc, str2);
    node3 = get_payload(tc, str3);
    node4 = get_payload(tc, str4);
    node5 = get_payload(tc, str5);
    config = get_config(tc, "10", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);

    result = icd_list__push(list, node1);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    result = icd_list__push(list, node2);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    result = icd_list__push(list, node3);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));
    result = icd_list__push(list, node4);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 4, icd_list__count(list));
    result = icd_list__push(list, node5);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 5, icd_list__count(list));

    x = 0;
    while (icd_list_iterator__has_more(iter)) {
        popped_payload = icd_list_iterator__next(iter);
        CuAssertPtrNotNull(tc, popped_payload);
        switch (x++) {
            case 0:
                CuAssertPtrEquals(tc, node1, popped_payload);
                break;
            case 1:
                CuAssertPtrEquals(tc, node2, popped_payload);
                break;
            case 2:
                CuAssertPtrEquals(tc, node3, popped_payload);
                break;
            case 3:
                CuAssertPtrEquals(tc, node4, popped_payload);
                break;
            case 4:
                CuAssertPtrEquals(tc, node5, popped_payload);
                break;
            default:
                CuFail(tc, "Too many nodes in list");
        }

    }
    destroy_icd_list_iterator(&iter);
    CuAssertPtrEquals(tc, NULL, iter);

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node1, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 4, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node2, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node3, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node4, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node5, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    destroy_payload(tc, node5);
    destroy_payload(tc, node4);
    destroy_payload(tc, node3);
    destroy_payload(tc, node2);
    destroy_payload(tc, node1);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test the LIFO behaviour.
 */
void test_icd_list__lifo(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    CuString *node1;
    CuString *node2;
    CuString *node3;
    CuString *node4;
    CuString *node5;
    CuString *popped_payload;
    char *str1 = "1";
    char *str2 = "2";
    char *str3 = "3";
    char *str4 = "4";
    char *str5 = "5";
    char *list_name = "test.lifo";

    node1 = get_payload(tc, str1);
    node2 = get_payload(tc, str2);
    node3 = get_payload(tc, str3);
    node4 = get_payload(tc, str4);
    node5 = get_payload(tc, str5);
    config = get_config(tc, "10", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    result = icd_list__set_node_insert_func(list, icd_list__insert_lifo, NULL);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    destroy_icd_config(&config);

    result = icd_list__push(list, node1);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    result = icd_list__push(list, node2);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    result = icd_list__push(list, node3);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node3, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));

    result = icd_list__push(list, node4);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 3, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node4, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node2, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));

    result = icd_list__push(list, node5);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 2, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node5, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 1, icd_list__count(list));
    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrNotNull(tc, popped_payload);
    CuAssertPtrEquals(tc, node1, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    popped_payload = (CuString *)icd_list__pop(list);
    CuAssertPtrEquals(tc, NULL, popped_payload);
    CuAssertIntEquals(tc, 10, icd_list__size(list));
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    CuAssertStrEquals(tc, list_name, icd_list__get_name(list));

    destroy_payload(tc, node5);
    destroy_payload(tc, node4);
    destroy_payload(tc, node3);
    destroy_payload(tc, node2);
    destroy_payload(tc, node1);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test the custom sort order behaviour (string order).
 */
void test_icd_list__customsort(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    CuString **nodeset;
    CuString *popped_payload;
    CuString *expected_payload;
    int nodes = 20;
    int x;
    int y = 0;
    char *list_name = "test.customsort";

    nodeset = get_many_payloads(tc, nodes, 1, 1);
    config = get_config(tc, "20", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    result = icd_list__set_node_insert_func(list, icd_list__insert_ordered, cmp_string_order);
    CuAssertTrue(tc, result == ICD_SUCCESS);
    destroy_icd_config(&config);

    for (x = 1; x < nodes; x += 2) {
        result = icd_list__push(list, nodeset[x]);
        CuAssertTrue(tc, result == ICD_SUCCESS);
    }
    CuAssertIntEquals(tc, 10, icd_list__count(list));
    for (x = 0; x < nodes; x += 2) {
        result = icd_list__push(list, nodeset[x]);
        CuAssertTrue(tc, result == ICD_SUCCESS);
    }
    CuAssertIntEquals(tc, 20, icd_list__count(list));

    /* x is the nodes in the list, y is the nodeset[] */
    /* Note that this needs mods if "nodes" changes */
    for (x = 0; x < nodes; x++) {
        switch (x) {
            case 0:   /* "1" */
                y = 0;
                break;
            case 1:   /* "10" */
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:  /* "19" */
                y = x + 8;
                break;
            case 11:  /* "2" */
                y = 1;
                break;
            case 12:  /* "20" */
                y = 19;
                break;
            case 13:  /* "3" */
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:  /* "9" */
                y = x - 11;
                break;
            default:
              CuFail(tc, "Warning: you must rework test_icd_list_customsort\n");
        }

        expected_payload = CuStringNew();
        CuAssertPtrNotNull(tc, expected_payload);
        CuStringAppendFormat(expected_payload, "%d", y + 1);

        popped_payload = (CuString *)icd_list__pop(list);
        CuAssertPtrNotNull(tc, popped_payload);
        CuAssertStrEquals(tc, expected_payload->buffer, popped_payload->buffer);
        CuAssertPtrEquals(tc, nodeset[y], popped_payload);

        CuStringDestroy(tc, expected_payload);
    }
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    destroy_many_payloads(tc, nodeset, nodes);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}


/*
 * Test another custom sort order behaviour (int order).
 */
void test_icd_list__customsort2(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    icd_status result;
    CuString **nodeset;
    CuString *popped_payload;
    CuString *expected_payload;
    int nodes = 20;
    int x;
    char *list_name = "test.customsort2";

    nodeset = get_many_payloads(tc, nodes, 1, 1);
    config = get_config(tc, "20", list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);
    result = icd_list__set_node_insert_func(list, icd_list__insert_ordered, cmp_int_order);
    CuAssertTrue(tc, result == ICD_SUCCESS);

    for (x = 1; x < nodes; x += 2) {
        result = icd_list__push(list, nodeset[x]);
        CuAssertTrue(tc, result == ICD_SUCCESS);
    }
    CuAssertIntEquals(tc, 10, icd_list__count(list));
    for (x = 0; x < nodes; x += 2) {
        result = icd_list__push(list, nodeset[x]);
        CuAssertTrue(tc, result == ICD_SUCCESS);
    }
    CuAssertIntEquals(tc, 20, icd_list__count(list));

    for (x = 0; x < nodes; x++) {
        expected_payload = CuStringNew();
        CuAssertPtrNotNull(tc, expected_payload);
        CuStringAppendFormat(expected_payload, "%d", x + 1);

        popped_payload = (CuString *)icd_list__pop(list);
        CuAssertPtrNotNull(tc, popped_payload);
        CuAssertStrEquals(tc, expected_payload->buffer, popped_payload->buffer);
        CuAssertPtrEquals(tc, nodeset[x], popped_payload);

        CuStringDestroy(tc, expected_payload);
    }
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    destroy_many_payloads(tc, nodeset, nodes);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test threading.
 */
void test_icd_list__threads(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    pthread_t *threads;
    sem_t finished_threads;
    sem_t init_threads;
    int finished_threadcount;
    CuString ***nodesets;
    CuString *expected_payload;
    CuString *popped_payload;
    thread_param **params;
    icd_status result;
    int threadcount = 10;
    int nodecount = 20;
    int listsize;
    int x;
    char *list_name = "test.threads";
    char size[6];

    sprintf(size, "%d", threadcount * nodecount);
    config = get_config(tc, size, list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);
    result = icd_list__set_node_insert_func(list, icd_list__insert_ordered, cmp_int_order);
    CuAssertTrue(tc, result == ICD_SUCCESS);

    threads = (pthread_t *)malloc(sizeof(pthread_t) * threadcount);
    CuAssertPtrNotNull(tc, threads);
    sem_init(&init_threads, 0, 0);
    sem_init(&finished_threads, 0, 0);

    nodesets = (CuString ***)malloc(sizeof(CuString **) * threadcount);
    CuAssertPtrNotNull(tc, nodesets);
    params = (thread_param **)malloc(sizeof(thread_param *) * threadcount);
    CuAssertPtrNotNull(tc, params);

    for (x = 0; x < threadcount; x++) {
        nodesets[x] = get_many_payloads(tc, nodecount, x + 1, threadcount);
        CuAssertPtrNotNull(tc, nodesets[x]);
        CuAssertPtrNotNull(tc, nodesets[x][0]);
        CuAssertPtrNotNull(tc, nodesets[x][nodecount - 1]);
        params[x] = get_thread_param(tc, x, nodesets[x], nodecount, threadcount, list, &init_threads, &finished_threads);
        CuAssertPtrNotNull(tc, params[x]);

        pthread_create(&(threads[x]), NULL, thread_fn, params[x]);
    }

    /* Wait for all threads to finish */
    sem_getvalue(&finished_threads, &finished_threadcount);
    while (finished_threadcount < threadcount) {
        sleep(1);
        sem_getvalue(&finished_threads, &finished_threadcount);
    }

    /* Check results */
    listsize = icd_list__count(list);
    CuAssertIntEquals(tc, nodecount * threadcount, listsize);
    for (x = 0; x < listsize; x++) {
        expected_payload = CuStringNew();
        CuAssertPtrNotNull(tc, expected_payload);
        CuStringAppendFormat(expected_payload, "%d", x + 1);

        popped_payload = (CuString *)icd_list__pop(list);
        CuAssertPtrNotNull(tc, popped_payload);
        CuAssertStrEquals(tc, expected_payload->buffer, popped_payload->buffer);
        /* ??? CuAssertPtrEquals(tc, nodesets[x], popped_payload); */

        CuStringDestroy(tc, expected_payload);
    }
    CuAssertIntEquals(tc, 0, icd_list__count(list));


    /* Clean up after ourselves */
    for (x = 0; x < threadcount; x++) {
        destroy_thread_param(tc, params[x]);
        destroy_many_payloads(tc, nodesets[x], nodecount);
    }
    free(params);
    free(nodesets);
    sem_destroy(&finished_threads);
    sem_destroy(&init_threads);
    free(threads);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/*
 * Test threading.
 */
void test_icd_list__iterate_remove_threads(CuTest* tc) {
    icd_list *list;
    icd_config *config;
    pthread_t *threads;
    sem_t finished_threads;
    sem_t init_threads;
    int finished_threadcount;
    CuString **nodeset;
    CuString *expected_payload;
    CuString *popped_payload;
    thread_param **params;
    icd_status result;
    int threadcount = 2;
    int nodecount = 50;
    int listsize;
    int x;
    char *list_name = "test.iterate-remove.threads";
    char size[6];

    sprintf(size, "%d", threadcount * nodecount);
    config = get_config(tc, size, list_name);
    list = create_icd_list(config);
    CuAssertPtrNotNull(tc, list);
    destroy_icd_config(&config);
    result = icd_list__set_node_insert_func(list, icd_list__insert_ordered, cmp_int_order);
    CuAssertTrue(tc, result == ICD_SUCCESS);

    threads = (pthread_t *)malloc(sizeof(pthread_t) * threadcount);
    CuAssertPtrNotNull(tc, threads);
    sem_init(&init_threads, 0, 0);
    sem_init(&finished_threads, 0, 0);

    params = (thread_param **)malloc(sizeof(thread_param *) * threadcount);
    CuAssertPtrNotNull(tc, params);
    nodeset = get_many_payloads(tc, nodecount, 1, 1);

    for (x = 1; x < nodecount; x++) {
        result = icd_list__push(list, nodeset[x]);
        CuAssertTrue(tc, result == ICD_SUCCESS);
    }

    params[0] = get_thread_param(tc, 0, nodeset, nodecount, threadcount, list, &init_threads, &finished_threads);
    CuAssertPtrNotNull(tc, params[0]);
    params[1] = get_thread_param(tc, 1, nodeset, nodecount, threadcount, list, &init_threads, &finished_threads);
    CuAssertPtrNotNull(tc, params[1]);


    pthread_create(&(threads[x]), NULL, removing_thread_fn, params[0]);
    pthread_create(&(threads[x]), NULL, iterating_thread_fn, params[1]);


    /* Wait for all threads to finish */
    sem_getvalue(&finished_threads, &finished_threadcount);
    while (finished_threadcount < threadcount) {
        sleep(1);
        sem_getvalue(&finished_threads, &finished_threadcount);
    }

    /* Check results */
    listsize = icd_list__count(list);
    CuAssertIntEquals(tc, 0, icd_list__count(list));

    /* Clean up after ourselves */
    destroy_many_payloads(tc, nodeset, nodecount);
    for (x = 0; x < threadcount; x++) {
        destroy_thread_param(tc, params[x]);
    }
    free(params);
    sem_destroy(&finished_threads);
    sem_destroy(&init_threads);
    free(threads);
    result = destroy_icd_list(&list);
    CuAssertTrue(tc, result == ICD_SUCCESS);
}

/* Generate the test suite */
CuSuite* test_icd_list__get_suite(void)
{
    CuSuite* suite = CuSuiteNew();
    registry = create_icd_config_registry("name");

    SUITE_ADD_TEST(suite, test_icd_list__create);
    SUITE_ADD_TEST(suite, test_icd_list__pushpop);
    SUITE_ADD_TEST(suite, test_icd_list__defaultfifo);
    SUITE_ADD_TEST(suite, test_icd_list__iterator);
    SUITE_ADD_TEST(suite, test_icd_list__fifo);
    SUITE_ADD_TEST(suite, test_icd_list__lifo);
    SUITE_ADD_TEST(suite, test_icd_list__customsort);
    SUITE_ADD_TEST(suite, test_icd_list__customsort2);
    SUITE_ADD_TEST(suite, test_icd_list__threads);
    SUITE_ADD_TEST(suite, test_icd_list__iterate_remove_threads);

    return suite;
}


