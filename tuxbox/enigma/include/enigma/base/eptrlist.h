#ifndef _E_PTRLIST_
#define _E_PTRLIST_

#include <list>
#include <vector>

template <class T>
class ePtrList : public std::list<T*>
{
public:
	typedef typename std::list<T*, std::allocator<T*> >::iterator std_list_T_iterator;  // to remove compiler warnings
	typedef typename std::list<T*, std::allocator<T*> >::const_iterator std_list_T_const_iterator;
	typedef typename std::list<T*, std::allocator<T*> >::reverse_iterator std_list_T_reverse_iterator;
	typedef typename std::list<T*, std::allocator<T*> >::const_reverse_iterator std_list_T_const_reverse_iterator;
	typedef typename ePtrList<T>::iterator T_iterator;
	typedef typename ePtrList<T>::const_iterator T_const_iterator;
	typedef typename ePtrList<T>::reverse_iterator T_reverse_iterator;
	typedef typename ePtrList<T>::const_reverse_iterator T_const_reverse_iterator;

// Iterator classes
	class iterator;
	class const_iterator;
	class reverse_iterator;
	class const_reverse_iterator;

// Constructors
	inline ePtrList();
	inline ePtrList(const ePtrList&);
	inline ~ePtrList();

// overwritted sort method
	inline void sort();

// changed methods for autodelete and current implementation
	inline void remove(T* t);
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
	inline T* setCurrent(const T*);
	inline const T* current() const;
	inline const T* next() const;
	inline const T* prev() const;
	inline const T* first() const;
	inline const T* last() const;

// added operator methods
	inline operator bool();
	inline bool operator!();

// added methods for autodelete implementation
	inline void setAutoDelete(bool b);
	inline bool isAutoDelete();

// added compare struct ... to sort
	struct less;
private:
	iterator cur;
	bool autoDelete;

public:
	iterator ePtrList<T>::begin()
	{				
	//	makes implicit type conversion form std::list::iterator to ePtrList::iterator
		return std::list<T*>::begin();		
	}

	iterator ePtrList<T>::end()
	{				
	//	makes implicit type conversion form std::list::iterator to ePtrList::iterator
		return std::list<T*>::end();		
	}

	const_iterator ePtrList<T>::begin() const
	{				
	//	makes implicit type conversion form std::list::const_iterator to ePtrList::const_iterator
		return std::list<T*>::begin();		
	}

	const_iterator ePtrList<T>::end() const
	{				
	//	makes implicit type conversion form std::list::const_iterator to ePtrList::const_iterator
		return std::list<T*>::end();		
	}

	reverse_iterator ePtrList<T>::rbegin()
	{				
	//	makes implicit type conversion form std::list::reverse:_iterator to ePtrList::reverse_iterator
		return std::list<T*>::rbegin();		
	}

	reverse_iterator ePtrList<T>::rend()
	{				
	//	makes implicit type conversion form std::list::reverse_iterator to ePtrList::reverse_iterator
		return std::list<T*>::rend();		
	}

	const_reverse_iterator ePtrList<T>::rbegin() const
	{				
	//	makes implicit type conversion form std::list::const_reverse_iterator to ePtrList::const_reverse_iterator
		return std::list<T*>::rbegin();		
	}

	const_reverse_iterator ePtrList<T>::rend() const
	{				
	//	makes implicit type conversion form std::list::const_reverse_iterator to ePtrList::const_reverse_iterator
		return std::list<T*>::rend();		
	}

	iterator ePtrList<T>::erase(iterator it)
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

	iterator ePtrList<T>::erase(iterator from, iterator to)
	{
	// 	Remove all items between the to iterators from and to
	//	If auto-deletion is enabled, than the list call delete for all removed items
		while (from != to)
			from = erase(from);
	
		return from;
	}

	operator iterator()	
	{
	//	Returns a iterator that equal to begin() of the list
		return begin();	
	}

	operator const_iterator() const
	{
	//	Returns a const_iterator that equal to begin() of the list
		return begin();	
	}

	operator reverse_iterator()
	{
	//	Returns a reverse_iterator that equal to rbegin() of the list
		return rbegin();	
	}

	operator const_reverse_iterator() const	
	{
	//	Returns a const_reverse_iterator that equal to rbegin() of the list
		return rbegin();	
	}

	std::vector<T>* getVector()
	{
		// Creates an vector and copys all elements to this vector
		// returns a pointer to this new vector ( the reserved memory must deletes from the receiver !! )
		std::vector<T>* v=new std::vector<T>();
		v->reserve( size() );
    for ( std_list_T_iterator it( std::list<T*>::begin() ); it != std::list<T*>::end(); it++)
			v->push_back( **it );

		return v;
	}

};

/////////////////// iterator class /////////////////////////////
template <class T>
class ePtrList<T>::iterator : public std::list<T*>::iterator
{
public:
	// Constructors
	iterator(const std_list_T_iterator& Q)		: std_list_T_iterator(Q)	{	}

	// changed operator for pointer
	T* operator->() const
	{
		return *std::list<T*>::iterator::operator->();
	}

	operator T&() const
	{
		return *operator->();
	}
	
	operator T*() const
	{
		return operator->();
	}
};

/////////////////// const_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::const_iterator : public std::list<T*>::const_iterator
{
public:
	// Constructors
	const_iterator(const std_list_T_const_iterator& Q)		:std_list_T_const_iterator(Q)	{	}

	// changed operator for pointer
	T* operator->() const
	{
		return *std::list<T*>::const_iterator::operator->();
	}

	operator T&() const
	{
		return *operator->();
	}

	operator T*() const
	{
		return operator->();
	}
};

/////////////////// reverse_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::reverse_iterator : public std::list<T*>::reverse_iterator
{
public:
	// Constructors
	reverse_iterator(const std_list_T_reverse_iterator& Q)		:std_list_T_reverse_iterator(Q)	{	}

	// changed operators for pointer
	T* operator->() const
	{
		return *std::list<T*>::reverse_iterator::operator->();
	}

	operator T&() const
	{
		return *operator->();
	}

	operator T*() const
	{
		return operator->();
	}
};

/////////////////// const_reverse_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::const_reverse_iterator : public std::list<T*>::const_reverse_iterator
{
public:
	// Constructors
	const_reverse_iterator(const std_list_T_const_reverse_iterator& Q)		:std_list_T_const_reverse_iterator(Q)	{	}

	// changed operators for pointer
	T* operator->() const
	{
		return *std::list<T*>::const_reverse_iterator::operator->();
	}

	operator T&() const
	{
		return *operator->();
	}

	operator T*() const
	{
		return operator->();
	}
};

/////////////////// Default Constructor /////////////////////////////
template <class T>
ePtrList<T>::ePtrList()
    :cur(std::list<T*>::begin()), autoDelete(false)		
{		

}

/////////////////// Copy Constructor /////////////////////////////
template <class T>
ePtrList<T>::ePtrList(const ePtrList& e)		
	:std::list<T*>(e), cur(e.cur), autoDelete(false)
{		

}

/////////////////// ePtrList Destructor /////////////////////////////
template <class T>
inline ePtrList<T>::~ePtrList()
{
// if autoDelete is enabled, delete is called for all elements in the list
	if (autoDelete)
		for (std_list_T_iterator it(std::list<T*>::begin()); it != std::list<T*>::end(); it++)
			delete *it;
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
	T_iterator it(std::list<T*>::begin());

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
inline T* ePtrList<T>::setCurrent(const T* t)
{
	// Sets the internal current iterator to the first element that equals to t, and returns t when a item is found,
	// otherwise it returns 0 !
	for (T_iterator it(std::list<T*>::begin()); it != std::list<T*>::end(); ++it)
		if (*it == t)
		{
			cur = it;
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
