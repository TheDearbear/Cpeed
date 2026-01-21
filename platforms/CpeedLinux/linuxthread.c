#include <Cpeed/platform/thread.h>

#include <malloc.h>
#include <pthread.h>

typedef struct CpdWrapperArgs {
    CpdThreadFunction function;
    void* context;
} CpdWrapperArgs;

void* pthread_wrapper(void* arg) {
    CpdWrapperArgs args = *(CpdWrapperArgs*)arg;
    free(arg);

    return (void*)(uintptr_t)args.function(args.context);
}

CpdThread create_thread(CpdThreadFunction func, void* context) {
    CpdWrapperArgs* args = (CpdWrapperArgs*)malloc(sizeof(CpdWrapperArgs));
    if (args == 0) {
        return 0;
    }

    args->function = func;
    args->context = context;

    pthread_t thread;
    int result = pthread_create(&thread, 0, pthread_wrapper, args);
    if (result != 0) {
        return 0;
    }

    return (CpdThread)thread;
}

CpdThread current_thread() {
    return (CpdThread)pthread_self();
}

void stop_current_thread(uint32_t code) {
    pthread_exit((void*)(uintptr_t)code);
}

bool join_thread(CpdThread handle, uint32_t* return_value) {
    void* thread_result = 0;
    int result = pthread_join((pthread_t)handle, &thread_result);

    if (result != 0 && return_value != 0) {
        *return_value = (uint32_t)(uintptr_t)thread_result;
    }

    return result == 0;
}

void detach_thread(CpdThread handle) {
    pthread_detach((pthread_t)handle);
}
