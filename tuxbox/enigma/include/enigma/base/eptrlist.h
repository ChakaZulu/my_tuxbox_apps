#ifndef _E_PTRLIST_
#define _E_PTRLIST_

#include <list>

template <class T>
class ePtrList : public std::list<T*>
{
	bool autoDelete;
	iterator cur;
public:
// Iterator classes
	class iterator;
	class const_iterator;
	class reverse_iterator;
	class const_reverse_iterator;

// Constructors
	inline ePtrList();
	inline ePtrList(const ePtrList&);
	inline ~ePtrList();

// overwritten methods
	inline iterator begin();
	inline iterator end();
	inline const_iterator begin() const;
	inline const_iterator end() const;
	inline reverse_iterator rbegin();
	inline reverse_iterator rend();
	inline const_reverse_iterator rbegin() const;
	inline const_reverse_iterator rend() const;

// overwritted sort method
	inline void sort();

// changed methods for autodelete and current implementation
	inline void remove(T* t);
	inline iterator erase(iterator it);
	inline iterator erase(iterator from, iterator to);
	inline void clear();
	inline void pop_back();
	inline void pop_front();
	inline void push_back(T*);
	inline void push_front(T*);

// added methods for current implementation
	inline T* take();
	inline void take(T* t);
	inline T* current();
	inline T* next();
	inline T* prev();
	inline T* first();
	inline T* last();
	inline T* setCurrent(const T*) const;
	inline const T* current() const;
	inline const T* next() const;
	inline const T* prev() const;
	inline const T* first() const;
	inline const T* last() const;

// added operator methods
	inline operator bool();
	inline bool operator!();
	inline operator iterator();
	inline operator const_iterator() const;
	inline operator reverse_iterator();
	inline operator const_reverse_iterator() const;

// added methods for autodelete implementation
	inline void setAutoDelete(bool b);
	inline bool isAutoDelete();

// added compare struct ... to sort
	struct less;
};

/////////////////// iterator class /////////////////////////////
template <class T>
class ePtrList<T>::iterator : public std::list<T*>::iterator
{
public:
	// Constructors
	iterator(const std::list<T*>::iterator& Q)		:std::list<T*>::iterator(Q)	{	}

	// changed operator for pointer
	T* operator->() const
	{
		return *std::list<T*>::iterator::operator->();
	}

	operator T&() const
	{
		return **std::list<T*>::iterator::operator->();
	}
	
	void operator+=(int i)
	{
		while (i--)
			operator++();
	}

	void operator-=(int i)
	{
		while (i--)
			operator--();
	}
};

/////////////////// const_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::const_iterator : public std::list<T*>::const_iterator
{
public:
	// Constructors
	const_iterator(const std::list<T*>::const_iterator& Q)		:std::list<T*>::const_iterator(Q)	{	}

	// changed operator for pointer
	T* operator->() const
	{
		return *std::list<T*>::const_iterator::operator->();
	}

	operator T&() const
	{
		return **std::list<T*>::const_iterator::operator->();
	}

	void operator+=(int i)
	{
		while (i--)
			operator++();
	}

	void operator-=(int i)
	{
		while (i--)
			operator--();
	}

};

/////////////////// reverse_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::reverse_iterator : public std::list<T*>::reverse_iterator
{
public:
	// Constructors
	reverse_iterator(const std::list<T*>::reverse_iterator& Q)		:std::list<T*>::reverse_iterator(Q)	{	}

	// changed operators for pointer
	T* operator->() const
	{
		return *std::list<T*>::reverse_iterator::operator->();
	}

	operator T&() const
	{
		return **std::list<T*>::reverse_iterator::operator->();
	}

	void operator+=(int i)
	{
		while (i--)
			operator++();
	}

	void operator-=(int i)
	{
		while (i--)
			operator--();
	}

};

/////////////////// const_reverse_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::const_reverse_iterator : public std::list<T*>::const_reverse_iterator
{
public:
	// Constructors
	const_reverse_iterator(const std::list<T*>::const_reverse_iterator& Q)		:std::list<T*>::const_reverse_iterator(Q)	{	}

	// changed operators for pointer
	T* operator->() const
	{
		return *std::list<T*>::const_reverse_iterator::operator->();
	}

	operator T&() const
	{
		return **std::list<T*>::const_reverse_iterator::operator->();
	}

	void operator+=(int i)
	{
		while (i--)
			operator++();
	}

	void operator-=(int i)
	{
		while (i--)
			operator--();
	}
};

/////////////////// Default Constructor /////////////////////////////
template <class T>
ePtrList<T>::ePtrList()		
	:autoDelete(false), cur(std::list<T*>::begin())		
{		

}

/////////////////// Copy Constructor /////////////////////////////
template <class T>
ePtrList<T>::ePtrList(const ePtrList& e)		
	:std::list<T*>(e), autoDelete(false), cur(e.cur)		
{		

}

/////////////////// ePtrList Destructor /////////////////////////////
template <class T>
inline ePtrList<T>::~ePtrList()
{
// if autoDelete is enabled, delete is called for all elements in the list
	if (autoDelete)
		for (std::list<T*>::iterator it(std::list<T*>::begin()); it != std::list<T*>::end(); it++)
			delete *it;
}

/////////////////// ePtrList begin() for iterator /////////////////////////////
template <class T>
inline ePtrList<T>::iterator ePtrList<T>::begin()
{				
//	makes implicit type conversion form std::list::iterator to ePtrList::iterator
	return std::list<T*>::begin();		
}

/////////////////// ePtrList end() for iterator /////////////////////////////
template <class T>
inline ePtrList<T>::iterator ePtrList<T>::end()
{				
//	makes implicit type conversion form std::list::iterator to ePtrList::iterator
	return std::list<T*>::end();		
}

/////////////////// ePtrList begin() for const_iterator /////////////////////////////
template <class T>
inline ePtrList<T>::const_iterator ePtrList<T>::begin() const
{				
//	makes implicit type conversion form std::list::const_iterator to ePtrList::const_iterator
	return std::list<T*>::begin();		
}

/////////////////// ePtrList end() for const_iterator /////////////////////////////
template <class T>
inline ePtrList<T>::const_iterator ePtrList<T>::end() const
{				
//	makes implicit type conversion form std::list::const_iterator to ePtrList::const_iterator
	return std::list<T*>::end();		
}

/////////////////// ePtrList rbegin() for reverse_iterator /////////////////////////////
template <class T>
inline ePtrList<T>::reverse_iterator ePtrList<T>::rbegin()
{				
//	makes implicit type conversion form std::list::reverse:_iterator to ePtrList::reverse_iterator
	return std::list<T*>::rbegin();		
}

/////////////////// ePtrList rend() for reverse_iterator /////////////////////////////
template <class T>
inline ePtrList<T>::reverse_iterator ePtrList<T>::rend()
{				
//	makes implicit type conversion form std::list::reverse_iterator to ePtrList::reverse_iterator
	return std::list<T*>::rend();		
}

/////////////////// ePtrList rbegin() for const_reverse_iterator /////////////////////////////
template <class T>
inline ePtrList<T>::const_reverse_iterator ePtrList<T>::rbegin() const
{				
//	makes implicit type conversion form std::list::const_reverse_iterator to ePtrList::const_reverse_iterator
	return std::list<T*>::rbegin();		
}

/////////////////// ePtrList rend() for const_reverse_iterator /////////////////////////////
template <class T>
inline ePtrList<T>::const_reverse_iterator ePtrList<T>::rend() const
{				
//	makes implicit type conversion form std::list::const_reverse_iterator to ePtrList::const_reverse_iterator
	return std::list<T*>::rend();		
}

/////////////////// ePtrList sort() /////////////////////////
template <class T>
inline void ePtrList<T>::sort()
{		
//	Sorts all items in the list.
// 	The type T must have a operator <.
	std::list<T*>::sort(ePtrList<T>::less());
}	

/////////////////// ePtrList remove(T*) /////////////////////////
template <class T>
inline void ePtrList<T>::remove(T* t)
{
// 	Remove all items that, equals to t, if auto-deletion is enabled, than the list call delete for the removed items
//  If current is equal to one of the removed items, current is set to the next valid item
	std::list<T*>::iterator it(std::list<T*>::begin());

	while (it != std::list<T*>::end())
		if (*it == t)
		{
			it=erase(it);
			break;  // one item is complete removed an deleted
		}
		else
			it++;
	
	while (it != std::list<T*>::end())
		if (*it == t)
			it = std::list<T*>::erase(it);  // remove all other items that equals to t (no delete is called..)
		else
			it++;
			
}

/////////////////// ePtrList erase(iterator) ///////////////////////
template <class T>
inline ePtrList<T>::iterator ePtrList<T>::erase(iterator it)
{
// 	Remove the item it, if auto-deletion is enabled, than the list call delete for this item
//  If current is equal to the item that was removed, current is set to the next item in the list
	if (autoDelete && *it)
		delete *it;

	if (cur == it)
		return cur = std::list<T*>::erase(it);
	else
		return std::list<T*>::erase(it);
}

/////////////////// ePtrList erase(iterator from. iterator to) //////////////////
template <class T>
inline ePtrList<T>::iterator ePtrList<T>::erase(iterator from, iterator to)
{
// 	Remove all items between the to iterators from and to
//	If auto-deletion is enabled, than the list call delete for all removed items
	while (from != to)
		from = erase(from);

	return from;
}

/////////////////// ePtrList clear() //////////////////
template <class T>
inline void ePtrList<T>::clear()	
{		
// 	Remove all items from the list
//	If auto-deletion is enabled, than the list call delete for all items in the list
	erase(std::list<T*>::begin(), std::list<T*>::end());	
}

/////////////////// ePtrList pop_back() ////////////////////
template <class T>
inline void ePtrList<T>::pop_back()
{
//	Removes the last item from the list. If the current item ist the last, than the current is set to the new
//	last item in the list;
//	The removed item is deleted if auto-deletion is enabled.
	erase(--end());
}

/////////////////// ePtrList pop_front() ////////////////////
template <class T>
inline void ePtrList<T>::pop_front()
{
//	Removes the first item from the list. If the current item ist the first, than the current is set to the new
//	first item in the list;
//	The removed item is deleted if auto-deletion is enabled.
	erase(begin());
}

/////////////////// ePtrList push_back(T*) ////////////////////
template <class T>
inline void ePtrList<T>::push_back(T* x)	
{		
// Add a new item at the end of the list.
// The current item is set to the last item;
	std::list<T*>::push_back(x);
	last();	
}

/////////////////// ePtrList push_front(T*) ////////////////////
template <class T>
inline void ePtrList<T>::push_front(T* x)	
{		
// Add a new item at the begin of the list.
// The current item is set to the first item;
	std::list<T*>::push_front(x);
	first();	
}

/////////////////// ePtrList take() ////////////////////
template <class T>
inline T* ePtrList<T>::take()
{
// Takes the current item out of the list without deleting it (even if auto-deletion is enabled).
// Returns a pointer to the item taken out of the list, or null if the index is out of range.
// The item after the taken item becomes the new current list item if the taken item is not the last item in the list. If the last item is taken, the new last item becomes the current item.
// The current item is set to null if the list becomes empty.
	T* tmp = *cur;
 	cur = std::list<T*>::erase(cur);
 	return tmp;
}

/////////////////// ePtrList take(T*) ////////////////////
template <class T>
inline void ePtrList<T>::take(T* t)
{
// Takes all item with T* out of the list without deleting it (even if auto-deletion is enabled).
 	std::list<T*>::remove(t);
}

/////////////////// ePtrList setCurrent(T*) ////////////////////
template <class T>
inline T* ePtrList<T>::setCurrent(const T* t) const
{
	// Sets the internal current iterator to the first element that equals to t, and returns t when a item is found,
	// otherwise it returns 0 !
	for (std::list<T*>::iterator it(std::list<T*>::begin()); it != std::list<T*>::end(); it++)
		if (*it == t)
		{
			current = it;
			return *it;
		}

	return 0;
}

/////////////////// ePtrList current() ////////////////////
template <class T>
inline T* ePtrList<T>::current()
{		
//	Returns a pointer to the current list item. The current item may be null (implies that the current index is -1).
	return cur==end() ? 0 : *cur;	
}

/////////////////// ePtrList next() ////////////////////
template <class T>
inline T* ePtrList<T>::next()
{		
//	Returns a pointer to the item succeeding the current item. Returns null if the current items is null or equal to the last item.
//	Makes the succeeding item current. If the current item before this function call was the last item, the current item will be set to null. If the current item was null, this function does nothing.
	if (cur == end())
		return 0;
	else
		if (++cur == end())
			return 0;
		else
			return *cur;
}

/////////////////// ePtrList prev() ////////////////////
template <class T>
inline T* ePtrList<T>::prev()
{		
//	Returns a pointer to the item preceding the current item. Returns null if the current items is null or equal to the first item.
//	Makes the preceding item current. If the current item before this function call was the first item, the current item will be set to null. If the current item was null, this function does nothing.
	if (cur == begin())
		return 0;
	else
		return *--cur;
}

/////////////////// ePtrList first() ////////////////////
template <class T>
inline T* ePtrList<T>::first()
{		
// Returns a pointer to the first item in the list and makes this the current list item, or null if the list is empty.
	return *(cur = begin());	
}

/////////////////// ePtrList last() ////////////////////
template <class T>
inline T* ePtrList<T>::last()
{		
//	Returns a pointer to the last item in the list and makes this the current list item, or null if the list is empty.
	return *(cur = --end());	
}

/////////////////// const ePtrList current() ////////////////////
template <class T>
inline const T* ePtrList<T>::current() const
{		
//	Returns a pointer to the current list item. The current item may be null (implies that the current index is not valid)
	return cur==end() ? 0 : *cur;	
}

/////////////////// const ePtrList next() ////////////////////
template <class T>
inline const T* ePtrList<T>::next() const
{		
//	Returns a pointer to the item succeeding the current item. Returns null if the current items is null or equal to the last item.
//	Makes the succeeding item current. If the current item before this function call was the last item, the current item will be set to null. If the current item was null, this function does nothing.
	if (cur == end())
		return 0;
	else
		if (++cur == end())
			return 0;
		else
			return *cur;
}

/////////////////// const ePtrList prev() ////////////////////
template <class T>
inline const T* ePtrList<T>::prev() const
{		
//	Returns a pointer to the item preceding the current item. Returns null if the current items is null or equal to the first item.
//	Makes the preceding item current. If the current item before this function call was the first item, the current item will be set to null. If the current item was null, this function does nothing.
	if (cur == begin())
		return 0;
	else
		return *--cur;
}

/////////////////// const ePtrList first() ////////////////////
template <class T>
inline const T* ePtrList<T>::first() const
{		
// Returns a pointer to the first item in the list and makes this the current list item, or null if the list is empty.
	return *(cur = begin());	
}

/////////////////// const ePtrList last() ////////////////////
template <class T>
inline const T* ePtrList<T>::last() const
{		
//	Returns a pointer to the last item in the list and makes this the current list item, or null if the list is empty.
	return *(cur = --end());	
}

////////////////// struct less //////////////////////////////
template <class T>
struct ePtrList<T>::less
{
// 	operator() is used internal from the list to sort them
	bool operator() (const T* t1, const T* t2)
	{
		return (*t1 < *t2);
	}
};

/////////////////// ePtrList operator bool ////////////////////
template <class T>
ePtrList<T>::operator bool()	
{
//	Returns a bool that contains true, when the list is NOT empty otherwise false
	return !empty();	
}

template <class T>
bool ePtrList<T>::operator!()	
{
//	Returns a bool that contains true, when the list is empty otherwise false
	return empty();	
}

template <class T>
ePtrList<T>::operator iterator()	
{
//	Returns a iterator that equal to begin() of the list
	return begin();	
}

template <class T>
ePtrList<T>::operator const_iterator() const
{
//	Returns a const_iterator that equal to begin() of the list
	return begin();	
}

template <class T>
ePtrList<T>::operator reverse_iterator()
{
//	Returns a reverse_iterator that equal to rbegin() of the list
	return rbegin();	
}

template <class T>
ePtrList<T>::operator const_reverse_iterator() const	
{
//	Returns a const_reverse_iterator that equal to rbegin() of the list
	return rbegin();	
}

template <class T>
void ePtrList<T>::setAutoDelete(bool b)
{
//	switched autoDelete on or off
// 	if autoDelete is true, than the pointer list controls the heap memory behind the pointer itself
//	the list calls delete for the item before it removed from the list
	autoDelete=b;	
}

template <class T>
bool ePtrList<T>::isAutoDelete()	
{		
// returns a bool that contains the state of autoDelete
	return autoDelete;	
}

#endif // _E_PTRLIST
