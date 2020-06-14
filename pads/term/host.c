#include "univ.h"
#define REMOTEBUFFER 64
unsigned char RemoteBuffer[REMOTEBUFFER];
int RemoteIndex;

void SendNChars(int n, uchar *p){
	int i;
	while (n)
		if ((i = write(hostfd[1], p, n)) > 0) {
			n -= i;
			p += i;
			continue;
		} else
			threadexitsall("SendNChars");
}

void FlushRemote(){
	if( RemoteIndex ){
		SendNChars(RemoteIndex, RemoteBuffer);
		RemoteIndex = 0;
	}
}

int GetRemote(void){
	int c;
	while( (c = rcvchar() ) == -1 ){
		extern int block;
		cursswitch(&Coffee);
                block = ~(1<<RHost);
                Jwait();
                block = 0;
		cursswitch(Jcursor);
	}
	return c;
}

void PutRemote( char c ){
	if( RemoteIndex >= REMOTEBUFFER ) FlushRemote();
	RemoteBuffer[RemoteIndex++] = c;
}

void ToHost( Protocol p, long l ){
	extern Pad *Current;

	void Proto(int);
	Proto( p );
	SendLong( HostParent->object );
	SendShort( HostParent->oid );
	SendLong( HostObject->object );
	SendShort( HostObject->oid );
	SendLong( l );
	FlushRemote();
}
	
void HostAction( Index *i ){	/* <text:action:opand> stored at host	*/
	ToHost( P_ACTION, *i );
}

void HostNumeric( long n ){	/* always in short range ! */
	ToHost( P_NUMERIC, (short) n );
}

void RCVServe(){	/* poll the host */ 
	Cursor *t = Jcursor;
	int op;

	FlushRemote();
Again:
	if( Pstate & (1<<RHost) ){
		op = GetRemote();
		switch( op&0xF0 ){
		case (int)P_VERSION&0xF0:
			if (op==(int)P_QUIT) exit(0);
			if( RcvLong() != PADS_VERSION )
			   ProtoErr( "host/term versions differ - regenerate. " );
			break;
		case (int)P_PICK&0xF0:
			PickOp();
			break;
		case (int)P_HOSTSTATE&0xF0:
			t = (Cursor*)0;
			if( op==(int)P_BUSY ){
				t = &HostBusy;
				goto Again;
			}
			break;
		case (int)P_PADDEF&0xF0:
		case (int)P_PADOP&0xF0:	
			PadOp(op);
			break;
		case (int)P_CACHEOP&0xF0:
			CacheOp(op);
			break;
		case (int)P_HELPSTR&0xF0:
			HelpString();
			break;
		default:
			ProtoErr( "RCVServe(): " );
		}
	}
	if( t != Jcursor ) cursswitch( Jcursor = t );
}

void HelpString(){
	char s[256];

	RcvString(s);
	InvertKBDrect(s,"");
}

void HostQuit(){
	ToHost( P_QUIT, /* garbage */ 0 );
}

void ProtoErr(char *s){
	extern char KBDStr[];
	char *k = KBDStr, *p = "protocol: ";

	while( *s ) *k++ = *s++;
	while( *p ) *k++ = *p++;
	for(;;){
		PaintKBD();
		if( k<&KBDStr[64] ) *k++ = GetRemote()&0x7F;
		sleep(30);
	}
}
