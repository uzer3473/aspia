//
// PROJECT:         Aspia
// FILE:            base/scoped_comptr.h
// LICENSE:         Mozilla Public License Version 2.0
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#ifndef _ASPIA_BASE__SCOPED_COMPTR_H
#define _ASPIA_BASE__SCOPED_COMPTR_H

#include <unknwn.h>

#include "base/logging.h"
#include "base/scoped_refptr.h"

namespace aspia {

// A fairly minimalistic smart class for COM interface pointers.
// Uses scoped_refptr for the basic smart pointer functionality
// and adds a few IUnknown specific services.
template <class Interface, const IID* interface_id = &__uuidof(Interface)>
class ScopedComPtr : public scoped_refptr<Interface>
{
public:
    // Utility template to prevent users of ScopedComPtr from calling AddRef
    // and/or Release() without going through the ScopedComPtr class.
    class BlockIUnknownMethods : public Interface
    {
    private:
        STDMETHOD(QueryInterface)(REFIID iid, void** object) = 0;
        STDMETHOD_(ULONG, AddRef)() = 0;
        STDMETHOD_(ULONG, Release)() = 0;
    };

    typedef scoped_refptr<Interface> ParentClass;

    ScopedComPtr() = default;

    explicit ScopedComPtr(Interface* p) : ParentClass(p) {}
    ScopedComPtr(const ScopedComPtr<Interface, interface_id>& p) : ParentClass(p) {}

    ~ScopedComPtr()
    {
        // We don't want the smart pointer class to be bigger than the pointer
        // it wraps.
        static_assert(sizeof(ScopedComPtr<Interface, interface_id>) == sizeof(Interface*),
                      "ScopedComPtrSize");
    }

    // Explicit Release() of the held object.  Useful for reuse of the
    // ScopedComPtr instance.
    // Note that this function equates to IUnknown::Release and should not
    // be confused with e.g. unique_ptr::release().
    void Release()
    {
        if (this->ptr_ != nullptr)
        {
            this->ptr_->Release();
            this->ptr_ = nullptr;
        }
    }

    // Sets the internal pointer to NULL and returns the held object without
    // releasing the reference.
    Interface* Detach()
    {
        Interface* p = this->ptr_;
        this->ptr_ = nullptr;
        return p;
    }

    // Accepts an interface pointer that has already been addref-ed.
    void Attach(Interface* p)
    {
        DCHECK(!this->ptr_);
        this->ptr_ = p;
    }

    // Retrieves the pointer address.
    // Used to receive object pointers as out arguments (and take ownership).
    // The function DCHECKs on the current value being NULL.
    // Usage: Foo(p.Receive());
    Interface** Receive()
    {
        DCHECK(!this->ptr_) << "Object leak. Pointer must be NULL";
        return &this->ptr_;
    }

    // A convenience for whenever a void pointer is needed as an out argument.
    void** ReceiveVoid()
    {
        return reinterpret_cast<void**>(Receive());
    }

    template <class Query>
    HRESULT QueryInterface(Query** p)
    {
        DCHECK(p != nullptr);
        DCHECK(this->ptr_ != nullptr);
        // IUnknown already has a template version of QueryInterface
        // so the iid parameter is implicit here. The only thing this
        // function adds are the DCHECKs.
        return this->ptr_->QueryInterface(p);
    }

    // QI for times when the IID is not associated with the type.
    HRESULT QueryInterface(const IID& iid, void** obj)
    {
        DCHECK(obj != nullptr);
        DCHECK(this->ptr_ != nullptr);
        return this->ptr_->QueryInterface(iid, obj);
    }

    // Queries |other| for the interface this object wraps and returns the
    // error code from the other->QueryInterface operation.
    HRESULT QueryFrom(IUnknown* object)
    {
        DCHECK(object != nullptr);
        return object->QueryInterface(Receive());
    }

    // Convenience wrapper around CoCreateInstance
    HRESULT CreateInstance(const CLSID& clsid, IUnknown* outer = nullptr,
                           DWORD context = CLSCTX_ALL)
    {
        DCHECK(!this->ptr_);
        HRESULT hr = ::CoCreateInstance(clsid, outer, context, *interface_id,
                                        reinterpret_cast<void**>(&this->ptr_));
        return hr;
    }

    // Checks if the identity of |other| and this object is the same.
    bool IsSameObject(IUnknown* other)
    {
        if (!other && !this->ptr_)
            return true;

        if (!other || !this->ptr_)
            return false;

        ScopedComPtr<IUnknown> my_identity;
        QueryInterface(my_identity.Receive());

        ScopedComPtr<IUnknown> other_identity;
        other->QueryInterface(other_identity.Receive());

        return my_identity == other_identity;
    }

    // Provides direct access to the interface.
    // Here we use a well known trick to make sure we block access to
    // IUnknown methods so that something bad like this doesn't happen:
    //    ScopedComPtr<IUnknown> p(Foo());
    //    p->Release();
    //    ... later the destructor runs, which will Release() again.
    // and to get the benefit of the DCHECKs we add to QueryInterface.
    // There's still a way to call these methods if you absolutely must
    // by statically casting the ScopedComPtr instance to the wrapped interface
    // and then making the call... but generally that shouldn't be necessary.
    BlockIUnknownMethods* operator->() const
    {
        DCHECK(this->ptr_ != nullptr);
        return reinterpret_cast<BlockIUnknownMethods*>(this->ptr_);
    }

    // Pull in operator=() from the parent class.
    using scoped_refptr<Interface>::operator=;

    // static methods

    static const IID& iid()
    {
        return *interface_id;
    }
};

}  // namespace aspia

#endif // _ASPIA_BASE__SCOPED_COMPTR_H
