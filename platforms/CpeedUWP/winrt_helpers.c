// MIT License
//
// Copyright (c) 2024 Leonard Hecker, and contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "winrt_helpers.h"

#include <Windows.Storage.Pickers.h>

typedef struct IAsyncOperationCompletedHandlerWrapper IAsyncOperationCompletedHandlerWrapper;
typedef struct IVectorView_HSTRING IVectorView_HSTRING;
typedef struct IVector_HSTRING IVector_HSTRING;

typedef struct IAsyncOperationCompletedHandlerWrapperVtable {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IAsyncOperationCompletedHandlerWrapper* self, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IAsyncOperationCompletedHandlerWrapper* self);
    ULONG(STDMETHODCALLTYPE* Release)(IAsyncOperationCompletedHandlerWrapper* self);
    HRESULT(STDMETHODCALLTYPE* Invoke)(IAsyncOperationCompletedHandlerWrapper* self, void* asyncInfo, AsyncStatus asyncStatus);
} IAsyncOperationCompletedHandlerWrapperVtable;

typedef struct IVector_HSTRING_Vtable {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IVector_HSTRING* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IVector_HSTRING* This);
    ULONG(STDMETHODCALLTYPE* Release)(IVector_HSTRING* This);
    HRESULT(STDMETHODCALLTYPE* GetIids)(IVector_HSTRING* This, ULONG* iidCount, IID** iids);
    HRESULT(STDMETHODCALLTYPE* GetRuntimeClassName)(IVector_HSTRING* This, HSTRING* className);
    HRESULT(STDMETHODCALLTYPE* GetTrustLevel)(IVector_HSTRING* This, TrustLevel* trustLevel);
    HRESULT(STDMETHODCALLTYPE* GetAt)(IVector_HSTRING* This, UINT32 index, HSTRING* result);
    HRESULT(STDMETHODCALLTYPE* get_Size)(IVector_HSTRING* This, UINT32* result);
    HRESULT(STDMETHODCALLTYPE* GetView)(IVector_HSTRING* This, IVectorView_HSTRING** result);
    HRESULT(STDMETHODCALLTYPE* IndexOf)(IVector_HSTRING* This, HSTRING value, UINT32* index, boolean* result);
    HRESULT(STDMETHODCALLTYPE* SetAt)(IVector_HSTRING* This, UINT32 index, HSTRING value);
    HRESULT(STDMETHODCALLTYPE* InsertAt)(IVector_HSTRING* This, UINT32 index, HSTRING value);
    HRESULT(STDMETHODCALLTYPE* RemoveAt)(IVector_HSTRING* This, UINT32 index);
    HRESULT(STDMETHODCALLTYPE* Append)(IVector_HSTRING* This, HSTRING value);
    HRESULT(STDMETHODCALLTYPE* RemoveAtEnd)(IVector_HSTRING* This);
    HRESULT(STDMETHODCALLTYPE* Clear)(IVector_HSTRING* This);
    HRESULT(STDMETHODCALLTYPE* GetMany)(IVector_HSTRING* This, UINT32 startIndex, UINT32 itemsLength, HSTRING* items, UINT32* result);
    HRESULT(STDMETHODCALLTYPE* ReplaceAll)(IVector_HSTRING* This, UINT32 itemsLength, HSTRING* items);
} IVector_HSTRING_Vtable;

struct IAsyncOperationCompletedHandlerWrapper {
    const IAsyncOperationCompletedHandlerWrapperVtable* lpVtbl;

    GUID iid;
    async_operation_callback callback;
    void* context;
    LONG ref_count;
};

struct IVector_HSTRING {
    const IVector_HSTRING_Vtable* lpVtbl;

    HSTRING* buffer;
    UINT32 capacity;
    UINT32 capacity_step;
    UINT32 size;
    LONG ref_count;
};

static HRESULT STDMETHODCALLTYPE IAsyncOperationCompletedHandlerWrapper_QueryInterface(IAsyncOperationCompletedHandlerWrapper* self, REFIID riid, void** ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IAgileObject) || IsEqualGUID(riid, &self->iid)) {
        *ppvObject = self;
        InterlockedIncrement(&self->ref_count);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE IAsyncOperationCompletedHandlerWrapper_AddRef(IAsyncOperationCompletedHandlerWrapper* self)
{
    return InterlockedIncrement(&self->ref_count);
}

static ULONG STDMETHODCALLTYPE IAsyncOperationCompletedHandlerWrapper_Release(IAsyncOperationCompletedHandlerWrapper* self)
{
    LONG ref_count = InterlockedDecrement(&self->ref_count);

    if (ref_count <= 0) {
        CoTaskMemFree(self);
    }

    return ref_count;
}

static HRESULT STDMETHODCALLTYPE IAsyncOperationCompletedHandlerWrapper_Invoke(IAsyncOperationCompletedHandlerWrapper* self, void* asyncInfo, AsyncStatus asyncStatus)
{
    return self->callback(self->context, asyncInfo, asyncStatus);
}

void* create_wrapper_for_IAsyncOperationCompletedHandler(REFIID iid, async_operation_callback callback, void* context)
{
    static const IAsyncOperationCompletedHandlerWrapperVtable lpVtbl = {
        .QueryInterface = IAsyncOperationCompletedHandlerWrapper_QueryInterface,
        .AddRef = IAsyncOperationCompletedHandlerWrapper_AddRef,
        .Release = IAsyncOperationCompletedHandlerWrapper_Release,
        .Invoke = IAsyncOperationCompletedHandlerWrapper_Invoke,
    };

    IAsyncOperationCompletedHandlerWrapper* wrapper = CoTaskMemAlloc(sizeof(IAsyncOperationCompletedHandlerWrapper));
    if (wrapper == NULL) {
        return NULL;
    }

    wrapper->lpVtbl = &lpVtbl;
    wrapper->iid = *iid;
    wrapper->callback = callback;
    wrapper->context = context;
    wrapper->ref_count = 0;
    return wrapper;
}

EXTERN_C const IID IID___FIVector_1_HSTRING = { 0x98B9ACC1, 0x4B56, 0x532E, 0xAC, 0x73, 0x03, 0xD5, 0x29, 0x1C, 0xCA, 0x90 };
EXTERN_C const IID IID___FIIterable_1_HSTRING = { 0xE2FCC7C1, 0x3BFC, 0x5A0B, 0xB2, 0xB0, 0x72, 0xE7, 0x69, 0xD1, 0xCB, 0x7E };

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_QueryInterface(IVector_HSTRING* self, REFIID riid, void** ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IAgileObject) || IsEqualGUID(riid, &IID_IInspectable) || IsEqualGUID(riid, &IID___FIIterable_1_HSTRING) || IsEqualGUID(riid, &IID___FIVector_1_HSTRING)) {
        *ppvObject = self;
        InterlockedIncrement(&self->ref_count);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE IVector_HSTRING_AddRef(IVector_HSTRING* self)
{
    return InterlockedIncrement(&self->ref_count);
}

static ULONG STDMETHODCALLTYPE IVector_HSTRING_Release(IVector_HSTRING* self)
{
    LONG ref_count = InterlockedDecrement(&self->ref_count);

    if (ref_count <= 0) {
        for (UINT32 i = 0; i < self->size; i++) {
            WindowsDeleteString(self->buffer[i]);
        }

        CoTaskMemFree(self->buffer);
        CoTaskMemFree(self);
    }

    return ref_count;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_GetIids(IVector_HSTRING* self, ULONG* count, IID** iids)
{
    if (count == NULL || iids == NULL) {
        return E_POINTER;
    }

    IID* iid = (IID*)CoTaskMemAlloc(sizeof(IID) * 3);
    if (iid == NULL) {
        return E_OUTOFMEMORY;
    }

    iid[0] = IID_IAgileObject;
    iid[1] = IID___FIIterable_1_HSTRING;
    iid[2] = IID___FIVector_1_HSTRING;

    *count = 3;
    *iids = iid;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_GetRuntimeClassName(IVector_HSTRING* self, HSTRING* className)
{
    if (className == NULL) {
        return E_POINTER;
    }

    static const WCHAR RuntimeClass_IVector_HSTRING[] = L"Windows.Foundation.Collections.IVector`1<String>";

    HSTRING name = 0;
    if (FAILED(WindowsCreateString(RuntimeClass_IVector_HSTRING, (UINT32)wcslen(RuntimeClass_IVector_HSTRING), &name))) {
        return E_OUTOFMEMORY;
    }

    *className = name;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_GetTrustLevel(IVector_HSTRING* self, TrustLevel* trustLevel)
{
    if (trustLevel == NULL) {
        return E_POINTER;
    }

    *trustLevel = BaseTrust;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_GetAt(IVector_HSTRING* self, UINT32 index, HSTRING* result)
{
    if (result == NULL) {
        return E_POINTER;
    }

    if (index >= self->size) {
        return E_BOUNDS;
    }

    *result = self->buffer[index];

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_get_Size(IVector_HSTRING* self, UINT32* result)
{
    if (result == NULL) {
        return E_POINTER;
    }

    *result = self->size;

    return S_OK;
}

// getview

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_IndexOf(IVector_HSTRING* self, HSTRING value, UINT32* index, boolean* result)
{
    if (index == NULL || result == NULL) {
        return E_POINTER;
    }

    UINT32 i = 0;
    boolean found = FALSE;

    for (; i < self->size; i++) {
        UINT32 result = 0;
        WindowsCompareStringOrdinal(value, self->buffer[i], &result);
        
        if (result == 0) {
            found = TRUE;
            break;
        }
    }
    
    *index = i;
    *result = found;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_SetAt(IVector_HSTRING* self, UINT32 index, HSTRING value)
{
    if (index >= self->size) {
        return E_BOUNDS;
    }

    if (self->buffer[index] == value) {
        return S_OK;
    }

    HSTRING new_value = 0;
    if (FAILED(WindowsDuplicateString(value, &new_value))) {
        return E_OUTOFMEMORY;
    }

    WindowsDeleteString(self->buffer[index]);

    self->buffer[index] = new_value;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_InsertAt(IVector_HSTRING* self, UINT32 index, HSTRING value)
{
    if (index > self->size) {
        return E_BOUNDS;
    }

    if (index == self->size) {
        return self->lpVtbl->Append(self, value);
    }

    if (self->size >= self->capacity) {
        UINT32 new_capacity = self->capacity + self->capacity_step;

        HSTRING* new_buffer = (HSTRING*)CoTaskMemRealloc(self->buffer, sizeof(HSTRING) * new_capacity);
        if (new_buffer == NULL) {
            return E_OUTOFMEMORY;
        }

        self->buffer = new_buffer;
        self->capacity = new_capacity;
    }


    HSTRING new_value = 0;
    if (FAILED(WindowsDuplicateString(value, &new_value))) {
        return E_OUTOFMEMORY;
    }

    for (UINT32 i = self->size - 1; i > index; i--) {
        self->buffer[i] = self->buffer[i - 1];
    }

    self->buffer[index] = new_value;
    self->size++;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_RemoveAt(IVector_HSTRING* self, UINT32 index)
{
    if (index >= self->size) {
        return E_BOUNDS;
    }

    WindowsDeleteString(self->buffer[index]);

    if (self->size + (2 * self->capacity_step) - 1 <= self->capacity) {
        UINT32 new_capacity = self->size + (2 * self->capacity_step) - 1;

        HSTRING* new_buffer = (HSTRING*)CoTaskMemRealloc(self->buffer, sizeof(HSTRING) * new_capacity);
        if (new_buffer == NULL) {
            return E_OUTOFMEMORY;
        }

        self->buffer = new_buffer;
        self->capacity = new_capacity;
    }


    for (UINT32 i = index; i < self->size - 1; i++) {
        self->buffer[i] = self->buffer[i + 1];
    }

    self->size--;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_Append(IVector_HSTRING* self, HSTRING value)
{
    if (self->size >= self->capacity) {
        UINT32 new_capacity = self->capacity + self->capacity_step;

        HSTRING* new_buffer = (HSTRING*)CoTaskMemRealloc(self->buffer, sizeof(HSTRING) * new_capacity);
        if (new_buffer == NULL) {
            return E_OUTOFMEMORY;
        }

        self->buffer = new_buffer;
        self->capacity = new_capacity;
    }

    HSTRING new_value = 0;
    if (FAILED(WindowsDuplicateString(value, &new_value))) {
        return E_OUTOFMEMORY;
    }

    self->buffer[self->size++] = new_value;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_RemoveAtEnd(IVector_HSTRING* self)
{
    if (self->size == 0) {
        return S_OK;
    }

    WindowsDeleteString(self->buffer[--self->size]);

    if (self->size + (2 * self->capacity_step) <= self->capacity) {
        UINT32 new_capacity = self->size + (2 * self->capacity_step);

        HSTRING* new_buffer = (HSTRING*)CoTaskMemRealloc(self->buffer, sizeof(HSTRING) * new_capacity);
        
        if (new_buffer != NULL) {
            self->buffer = new_buffer;
            self->capacity = new_capacity;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_Clear(IVector_HSTRING* self)
{
    for (UINT32 i = 0; i < self->size; i++) {
        WindowsDeleteString(self->buffer[i]);
    }

    self->size = 0;

    if (2 * self->capacity_step <= self->capacity) {
        UINT32 new_capacity = 2 * self->capacity_step;

        HSTRING* new_buffer = (HSTRING*)CoTaskMemRealloc(self->buffer, sizeof(HSTRING) * new_capacity);
        
        if (new_buffer != NULL) {
            self->buffer = new_buffer;
            self->capacity = new_capacity;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_GetMany(IVector_HSTRING* self, UINT32 startIndex, UINT32 itemsLength, HSTRING* items, UINT32* result)
{
    if (startIndex >= self->size) {
        return E_BOUNDS;
    }

    UINT32 copy_count = self->size - startIndex - 1;
    if (itemsLength < copy_count) {
        copy_count = itemsLength;
    }

    for (UINT32 i = 0; i < copy_count; i++) {
        items[i] = self->buffer[startIndex + i];
    }

    *result = copy_count;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE IVector_HSTRING_ReplaceAll(IVector_HSTRING* self, UINT32 itemsLength, HSTRING* items)
{
    HSTRING* old_strings = (HSTRING*)CoTaskMemAlloc(sizeof(HSTRING) * self->size);
    if (old_strings == NULL) {
        return E_OUTOFMEMORY;
    }

    for (UINT32 i = 0; i < self->size; i++) {
        old_strings[i] = self->buffer[i];
    }

    if (self->size > itemsLength) {
        for (UINT32 i = 0; i < itemsLength; i++) {
            HSTRING new_item = 0;
            if (FAILED(WindowsDuplicateString(items[i], &new_item))) {
                for (UINT32 j = 0; j < i; j++) {
                    WindowsDeleteString(self->buffer[j]);
                    self->buffer[j] = old_strings[j];
                }

                CoTaskMemFree(old_strings);

                return E_OUTOFMEMORY;
            }

            self->buffer[i] = new_item;
        }
    }

    HSTRING* new_buffer = (HSTRING*)CoTaskMemRealloc(self->buffer, sizeof(HSTRING) * itemsLength);
    if (new_buffer == NULL) {
        return E_OUTOFMEMORY;
    }

    if (self->size <= itemsLength) {
        for (UINT32 i = 0; i < itemsLength; i++) {
            HSTRING new_item = 0;
            if (FAILED(WindowsDuplicateString(items[i], &new_item))) {
                for (UINT32 j = 0; j < i; j++) {
                    WindowsDeleteString(self->buffer[j]);

                    if (j < self->size) {
                        self->buffer[j] = old_strings[j];
                    }
                }

                CoTaskMemFree(old_strings);

                return E_OUTOFMEMORY;
            }

            self->buffer[i] = new_item;
        }
    }

    for (UINT32 i = 0; i < self->size; i++) {
        WindowsDeleteString(old_strings[i]);
    }

    CoTaskMemFree(old_strings);

    self->buffer = new_buffer;
    self->capacity = itemsLength;
    self->size = itemsLength;

    return S_OK;
}

void* create_wrapper_for_IVector_HSTRING()
{
    static const IVector_HSTRING_Vtable lpVtbl = {
        .QueryInterface = IVector_HSTRING_QueryInterface,
        .AddRef = IVector_HSTRING_AddRef,
        .Release = IVector_HSTRING_Release,
        .GetIids = IVector_HSTRING_GetIids,
        .GetRuntimeClassName = IVector_HSTRING_GetRuntimeClassName,
        .GetTrustLevel = IVector_HSTRING_GetTrustLevel,
        .GetAt = IVector_HSTRING_GetAt,
        .get_Size = IVector_HSTRING_get_Size,
        .GetView = 0, // IVector_HSTRING_GetView,
        .IndexOf = IVector_HSTRING_IndexOf,
        .SetAt = IVector_HSTRING_SetAt,
        .InsertAt = IVector_HSTRING_InsertAt,
        .RemoveAt = IVector_HSTRING_RemoveAt,
        .Append = IVector_HSTRING_Append,
        .RemoveAtEnd = IVector_HSTRING_RemoveAtEnd,
        .Clear = IVector_HSTRING_Clear,
        .GetMany = IVector_HSTRING_GetMany,
        .ReplaceAll = IVector_HSTRING_ReplaceAll
    };

    IVector_HSTRING* wrapper = CoTaskMemAlloc(sizeof(IVector_HSTRING));
    if (wrapper == NULL) {
        return NULL;
    }

    wrapper->lpVtbl = &lpVtbl;
    wrapper->buffer = 0;
    wrapper->capacity = 0;
    wrapper->capacity_step = 8;
    wrapper->size = 0;
    wrapper->ref_count = 0;
    return wrapper;
}
