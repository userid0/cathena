#include "utils.h"
#include "showmsg.h"
#include "malloc.h"
#include "mmo.h"

void dump(unsigned char *buffer, size_t num)
{
	size_t icnt,jcnt;
	ShowMessage("         Hex                                                  ASCII\n");
	ShowMessage("         -----------------------------------------------      ----------------");
	
	for (icnt=0;icnt<num;icnt+=16)
	{
		ShowMessage("\n%p ",&buffer[icnt]);
		for (jcnt=icnt;jcnt<icnt+16;++jcnt)
		{
			if (jcnt < num)
				ShowMessage("%02hX ",buffer[jcnt]);
			else
				ShowMessage("   ");
		}
		ShowMessage("  |  ");
		for (jcnt=icnt;jcnt<icnt+16;++jcnt)
		{
			if (jcnt < num)
			{
				if (buffer[jcnt] > 31 && buffer[jcnt] < 127)
					ShowMessage("%c",buffer[jcnt]);
				else
					ShowMessage(".");
			}
			else
				ShowMessage(" ");
		}
	}
	ShowMessage("\n");
}

// Allocate a StringBuf  [MouseJstr]
struct StringBuf * StringBuf_Malloc() 
{
	struct StringBuf * ret = (struct StringBuf *)aMalloc(sizeof(struct StringBuf));
	StringBuf_Init(ret);
	return ret;
}

// Initialize a previously allocated StringBuf [MouseJstr]
void StringBuf_Init(struct StringBuf * sbuf)  {
	sbuf->max_ = 1024;
	sbuf->ptr_ = sbuf->buf_ = (char *)aMalloc((sbuf->max_ + 1)*sizeof(char));
}

// printf into a StringBuf, moving the pointer [MouseJstr]
int StringBuf_Printf(struct StringBuf *sbuf,const char *fmt,...) 
{
	va_list ap;
	int n, size, off;
	
	
	while (1) {
		/* Try to print in the allocated space. */
		size = sbuf->max_ - (sbuf->ptr_ - sbuf->buf_);
		va_start(ap, fmt);
		n = vsnprintf (sbuf->ptr_, size, fmt, ap);
		va_end(ap);

		/* If that worked, return the length. */
		if (n > -1 && n < size) {
			sbuf->ptr_ += n;
			break;
		}
		/* Else try again with more space. */
		sbuf->max_ *= 2; // twice the old size
		off = sbuf->ptr_ - sbuf->buf_;
		sbuf->buf_ = (char *) aRealloc(sbuf->buf_, (sbuf->max_ + 1)*sizeof(char));
		sbuf->ptr_ = sbuf->buf_ + off;
	}
	
	return sbuf->ptr_ - sbuf->buf_;
}

// Append buf2 onto the end of buf1 [MouseJstr]
int StringBuf_Append(struct StringBuf *buf1,const struct StringBuf *buf2) 
{
	int buf1_avail = buf1->max_ - (buf1->ptr_ - buf1->buf_);
	int size2 = buf2->ptr_ - buf2->buf_;

	if (size2 >= buf1_avail)  {
		int off = buf1->ptr_ - buf1->buf_;
		buf1->max_ += size2;
		buf1->buf_ = (char *) aRealloc(buf1->buf_, (buf1->max_ + 1)*sizeof(char));
		buf1->ptr_ = buf1->buf_ + off;
	}

	memcpy(buf1->ptr_, buf2->buf_, size2);
	buf1->ptr_ += size2;
	return buf1->ptr_ - buf1->buf_;
}

// Destroy a StringBuf [MouseJstr]
void StringBuf_Destroy(struct StringBuf *sbuf) 
{
	aFree(sbuf->buf_);
	sbuf->ptr_ = sbuf->buf_ = 0;
}

// Free a StringBuf returned by StringBuf_Malloc [MouseJstr]
void StringBuf_Free(struct StringBuf *sbuf) 
{
	StringBuf_Destroy(sbuf);
	aFree(sbuf);
}

// Return the built string from the StringBuf [MouseJstr]
char * StringBuf_Value(struct StringBuf *sbuf) 
{
	*sbuf->ptr_ = '\0';
	return sbuf->buf_;
}


int config_switch(const char *str) {
	if (strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "oui") == 0 || strcasecmp(str, "ja") == 0 || strcasecmp(str, "si") == 0)
		return 1;
	if (strcasecmp(str, "off") == 0 || strcasecmp(str, "no" ) == 0 || strcasecmp(str, "non") == 0 || strcasecmp(str, "nein") == 0)
		return 0;

	return atoi(str);
}
///////////////////////////////////////////////////////////////////////////
// converts a string to an ip (host byte order)
uint32 str2ip(const char *str)
{
	struct hostent*h;
	while( isspace( ((unsigned char)(*str)) ) ) str++;
	h = gethostbyname(str);
	if (h != NULL)
		return ipaddress( MakeDWord((unsigned char)h->h_addr[3], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[0]) );
	else
		return ipaddress( ntohl(inet_addr(str)) );
}



//-----------------------------------------------------
// Function to suppress control characters in a string.
//-----------------------------------------------------
bool remove_control_chars(char *str)
{
	bool change = false;
	if(str)
	while( *str )
	{	// replace control chars 
		// but skip chars >0x7F which are negative in char representations
		if ( (*str<32) && (*str>0) )
		{
			*str = '_';
			change = true;
		}
		str++;
	}
	return change;
}

//------------------------------------------------------------
// E-mail check: return 0 (not correct) or 1 (valid). by [Yor]
//------------------------------------------------------------
bool email_check(const char *email) {
	char ch;
	const char* last_arobas;

	// athena limits
	if (!email || strlen(email) < 3 || strlen(email) > 39)
		return false;

	// part of RFC limits (official reference of e-mail description)
	if (strchr(email, '@') == NULL || email[strlen(email)-1] == '@')
		return false;

	if (email[strlen(email)-1] == '.')
		return false;

	last_arobas = strrchr(email, '@');

	if (strstr(last_arobas, "@.") != NULL ||
	    strstr(last_arobas, "..") != NULL)
		return false;

	for(ch = 1; ch < 32; ch++) {
		if (strchr(last_arobas, ch) != NULL) {
			return false;
		}
	}

	if (strchr(last_arobas, ' ') != NULL ||
	    strchr(last_arobas, ';') != NULL)
		return false;

	// all correct
	return true;
}















