#ifndef ERECT_H
#define ERECT_H

#include <lib/base/esize.h>
#include <lib/base/epoint.h>


// x2 = x1 + width  (AND NOT, NEVER, NEVER EVER +1 or -1 !!!!)

class eRect // rectangle class
{
public:
	eRect()	{ x1 = y1 = x2 = y2 = 0; }
	eRect( const ePoint &topleft, const ePoint &bottomright );

	// we use this contructor very often... do it inline...
	eRect( const ePoint &topleft, const eSize &size )
	{
		x1 = topleft.x();
		y1 = topleft.y();
		x2 = (x1+size.width());
		y2 = (y1+size.height());
	}

	eRect( int left, int top, int width, int height );

	bool isNull()	const;
	bool isEmpty()	const;
	bool isValid()	const;
	eRect normalize()	const;

	int left()	const;
	int top()	const;
	int right()	const;
	int  bottom()	const;
	int &rLeft();
	int &rTop();
	int &rRight();
	int &rBottom();

	int x() const;
	int y() const;
	void setLeft( int pos );
	void setTop( int pos );
	void setRight( int pos );
	void setBottom( int pos );
	void setX( int x );
	void setY( int y );

	ePoint topLeft()	 const;
	ePoint bottomRight() const;
	ePoint topRight()	 const;
	ePoint bottomLeft()	 const;
	ePoint center()	 const;

	void rect( int *x, int *y, int *w, int *h ) const;
	void coords( int *x1, int *y1, int *x2, int *y2 ) const;

	void moveTopLeft( const ePoint &p );
	void moveBottomRight( const ePoint &p );
	void moveTopRight( const ePoint &p );
	void moveBottomLeft( const ePoint &p );
	void moveCenter( const ePoint &p );

	void moveBy( int dx, int dy )
	{
		x1 += dx;
		y1 += dy;
		x2 += dx;
		y2 += dy;
	}

	void setRect( int x, int y, int w, int h );
	void setCoords( int x1, int y1, int x2, int y2 );

	eSize size()	const;
	int width()	const;
	int height()	const;
	void setWidth( int w );
	void setHeight( int h );
	void setSize( const eSize &s );

	eRect operator|(const eRect &r) const;
	eRect operator&(const eRect &r) const;
 	eRect& operator|=(const eRect &r);
 	eRect& operator&=(const eRect &r);

	bool contains( const ePoint &p) const;
	bool contains( int x, int y) const;
	bool contains( const eRect &r) const;
	eRect unite( const eRect &r ) const;
	eRect intersect( const eRect &r ) const;
	bool intersects( const eRect &r ) const;

	friend bool operator==( const eRect &, const eRect & );
	friend bool operator!=( const eRect &, const eRect & );

private:
	int x1;
	int y1;
	int x2;
	int y2;
};

bool operator==( const eRect &, const eRect & );
bool operator!=( const eRect &, const eRect & );


/*****************************************************************************
  eRect stream functions
 *****************************************************************************/
namespace std
{
	inline ostream &operator<<( ostream & s, const eRect & r )
	{
		s << r.left()  << r.top()
		  << r.right() << r.bottom();

		return s;
	}

	inline istream &operator>>( istream & s, eRect & r )
	{
		int x1, y1, x2, y2;
		s >> x1 >> y1 >> x2 >> y2;
		r.setCoords( x1, y1, x2, y2 );
		return s;
	}
}

/*****************************************************************************
  eRect inline member functions
 *****************************************************************************/

inline eRect::eRect( int left, int top, int width, int height )
{
	x1 = left;
	y1 = top;
	x2 = left+width;
	y2 = top+height;
}

inline bool eRect::isNull() const
{ return x2 == x1 && y2 == y1; }

inline bool eRect::isEmpty() const
{ return x1 >= x2 || y1 >= y2; }

inline bool eRect::isValid() const
{ return x1 <= x2 && y1 <= y2; }

inline int eRect::left() const
{ return x1; }

inline int eRect::top() const
{ return y1; }

inline int eRect::right() const
{ return x2; }

inline int eRect::bottom() const
{ return y2; }

inline int &eRect::rLeft()
{ return x1; }

inline int & eRect::rTop()
{ return y1; }

inline int & eRect::rRight()
{ return x2; }

inline int & eRect::rBottom()
{ return y2; }

inline int eRect::x() const
{ return x1; }

inline int eRect::y() const
{ return y1; }

inline void eRect::setLeft( int pos )
{ x1 = pos; }

inline void eRect::setTop( int pos )
{ y1 = pos; }

inline void eRect::setRight( int pos )
{ x2 = pos; }

inline void eRect::setBottom( int pos )
{ y2 = pos; }

inline void eRect::setX( int x )
{ x1 = x; }

inline void eRect::setY( int y )
{ y1 = y; }

inline ePoint eRect::topLeft() const
{ return ePoint(x1, y1); }

inline ePoint eRect::bottomRight() const
{ return ePoint(x2, y2); }

inline ePoint eRect::topRight() const
{ return ePoint(x2, y1); }

inline ePoint eRect::bottomLeft() const
{ return ePoint(x1, y2); }

inline ePoint eRect::center() const
{ return ePoint((x1+x2)/2, (y1+y2)/2); }

inline int eRect::width() const
{ return  x2 - x1; }

inline int eRect::height() const
{ return  y2 - y1; }

inline eSize eRect::size() const
{ return eSize(x2-x1, y2-y1); }

inline bool eRect::contains( int x, int y) const
{
	return x >= x1 && x < x2 && y >= y1 && y < y2;
}

#endif // eRect_H
