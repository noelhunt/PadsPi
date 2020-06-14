#include <pads.h>
SRCFILE("remote.c")

void Remote::checkproto(int p)		{ if( get()!=p ) err(); }
void Remote::proto(int p)		{ put( p ); }

long Remote::rcvlong()			{ return (long)  shiftin( P_LONG  ); }
short Remote::rcvshort()		{ return (short) shiftin( P_SHORT ); }
unsigned char Remote::rcvuchar()	{ return (uchar) shiftin(P_UCHAR); }

void Remote::sendlong(long  x)		{ shiftout( P_LONG, x );	 }
void Remote::sendshort(short x)		{ shiftout( P_SHORT, (long) x ); }
void Remote::senduchar(uchar x)		{ shiftout( P_UCHAR, (long) x ); }
void Remote::pktflush()			{ writesize = 0; pktend(); 	}

void Remote::pktstart(char c){
	put(c);
}

void Remote::put(char c){
	writebuffer[pktsize++] = c;
	if (pktsize == sizeof(writebuffer))
		pktflush();
}

void Remote::sendobj(PadRcv *o){
	sendlong((long)o);
}

PadRcv *Remote::rcvobj(){
	PadRcv *obj = (PadRcv*)rcvlong();
	short oid = rcvshort();
	if (obj && obj->oid != oid)
		obj = 0;
	trace("0x%08x.rcvobj() obj.0x%08x", this, obj);
	return obj;
}

void Remote::err(const char *e){
	if( !e ) e = "Pads library: protocol error";
	PadsError(e);
}

Remote::Remote(int rd, int wr){
	ifd = rd;
	ofd = wr;
	pktsize = writesize = 0;
}

Remote::Remote(char *dev){
	ifd = ofd = open(dev, 2);
	pktsize = writesize = 0;
}

void Remote::share(){
	trace( "%d.share()", this );
}	

long Remote::shiftin(int bytes){
	long shifter = 0;

	trace("0x%08x.shiftin(bytes.%d)", this, bytes);
	checkproto( bytes );
	while( bytes-- ) shifter = (shifter<<8) + (get()&0xFF);
	return shifter;
}

void Remote::shiftout( int bytes, long shifter ){
	trace("0x%08x.shiftout(bytes.%d, shifter.%d)",this,bytes,shifter);
	proto( bytes );
	do { put( (char)(shifter>>( (--bytes)*8 )) ); } while( bytes );
}

long BytesToTerm;
void Remote::pktend(){
	if (pktsize > writesize) {
		if (write(ofd, (char*)writebuffer, pktsize) != pktsize)
			abort();
		BytesToTerm += pktsize;
		pktsize = 0;
		writesize = sizeof(writebuffer);
	}
}

char *Remote::rcvstring( char *s0 ){
	register char *s = s0;
	register unsigned char len;

	checkproto( P_STRING );
	len = rcvuchar();
	if( !s0 ) s = s0 = new char [len+1];
	while( len-->0 ) *s++ = get();
	*s = '\0';
	return s0;
}

void Remote::sendstring(const char *s){
	int len;

	trace( "0x%08x.sendstring(%s)", this, s );
	proto( P_STRING );
	len =  strlen(s);
	if( len > 255 ) len = 255;
	senduchar( len );
	while( len-- ) put(*s++); 
}

#include <errno.h>

long BytesFromTerm;
#ifdef notdef
int Remote::get(){
	char c;
	trace("0x%08x.get pktsize.%d", this, pktsize);
	if (pktsize) {
		err();
		return 0;
	}
	while (read(fd, &c, 1) != 1) {
		if (errno == EINTR)
			continue;
		err();
	}
	++BytesFromTerm;
	return c;
}
#else
int Remote::get(){
	static uchar buf[128];
	static int i, nleft = 0;

	if (pktsize) {
		err("Remote::get: pktsize");
		return 0;
	}
	if( nleft <= 0 ){
again:
		if( (nleft = read(ifd, (char *)buf, sizeof buf)) < 0 ){
			if(errno == EINTR)	/* why are we getting EINTR? */
				goto again;
			err(strerror(errno));
			return 0;
		}
		i = 0;
	}
	++BytesFromTerm, --nleft;
	return (int)buf[i++];
}
#endif
