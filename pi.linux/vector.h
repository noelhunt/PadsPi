#ifndef VECTOR_H
#define VECTOR_H

/* sccsid[] = "%W%\t%G%" */

/*
 * Tom Cargill. 1996. A dynamic vector is harder than it looks. In C++ Gems,
 *	Stanley B. Lippman (Ed.). Sigs Reference Library Series, Vol. 5.
 *	SIGS Publications, Inc., New York, NY, USA 185-193.
 *
 * @incollection{Cargill:1996:DVH:260627.260654,
 *     author = {Cargill, Tom},
 *    chapter = {A Dynamic Vector is Harder Than It Looks},
 *      title = {C++ Gems},
 *     editor = {Lippman, Stanley B.},
 *       year = {1996},
 *       isbn = {1-884842-37-2},
 *      pages = {185--193},
 *   numpages = {9},
 *        url = {http://dl.acm.org/citation.cfm?id=260627.260654},
 *      acmid = {260654},
 *  publisher = {SIGS Publications, Inc.},
 *    address = {New York, NY, USA},
 * } 
 */

typedef unsigned int	uint;
typedef unsigned char	uchar;

template <class T>
class  Vector {
	int	bucket;
	int	mask;
	uint	_size;
	uint	_capacity;
	int	log2sz;
	T**	ptr;
	void	log2(uint);
	void	grow();
public:
		~Vector();
		Vector();
		Vector(const Vector& v);
	Vector&	operator = (const Vector&);
	uint	capacity() const;
	uint	size() const;
	bool	empty() const;
	T&	end();
	void	push(const T& value);
	void	pop();
	T&	operator[](uint);
	T&	at(uint);
	void	clear();
};

template <class T>
Vector<T>::~Vector() {
	while( log2sz >= 0 )
		delete [] ptr[log2sz--];
	delete [] ptr;
}

template <class T>
Vector<T>::Vector() {
	ptr = new T*[1];
	ptr[0] = new T[1];
	_size = 0;
	log2sz = 0;
	_capacity = 1;
}

template <class T>
T& Vector<T>::end() {
	int last = _size-1;
	log2(last);
	return ptr[bucket][last&(mask-1)];
}

template <class T>
bool Vector<T>::empty() const {
	return _size == 0;
}

template <class T>
uint Vector<T>::size() const {
	return _size;
}

template <class T>
uint Vector<T>::capacity() const {
	return _capacity;
}

template <class T>
T& Vector<T>::operator[](uint index) {
	log2(index);
	return ptr[bucket][index&(mask-1)];
}

template <class T>
T& Vector<T>::at(uint index) {
	log2(index);
	return ptr[bucket][index&(mask-1)];
}

template <class T>
void Vector<T>::grow() {
	T** p = new T*[++log2sz+1];
	for (int i=0; i<log2sz; i++)
		p[i] = ptr[i];
	delete [] ptr;
	ptr = p;
	ptr[log2sz] = new T[_capacity];
	_capacity *= 2;
}

template <class T>
void Vector<T>::push(const T& v) {
	while (_size >= _capacity)
		grow();
	log2(_size);
	ptr[bucket][_size&(mask-1)] = v;
	_size++;
}

template <class T>
void Vector<T>::pop() {
	_size--;
}

template <class T>
void Vector<T>::clear() {
	_size = 0;
	log2sz = 0;
	_capacity = 0;
	while( log2sz >= 0 )
		delete [] ptr[log2sz--];
	delete [] ptr;
	ptr = 0;
}

template <class T>
void Vector<T>::log2(uint index){
	bucket = mask = 0;
	while (index >= 1<<bucket )
		mask = 1<<bucket++;
}
#endif
