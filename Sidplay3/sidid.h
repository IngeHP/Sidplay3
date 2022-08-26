#ifndef _SIDID_H_
#define _SIDID_H_

#include <vector>
using std::vector;

#define MAX_SIGSIZE 4096

class SIDID
{
public:
	SIDID();
	SIDID(char *configname);
	~SIDID();

	int identifyfile(char *name);
	int identifybuffer(unsigned char *buffer, int length);
	operator bool () { return status; }

	char playersfound[4096];

private:
	struct sSIDID
	{
		char *name;
		int count;
		void *firstsig;
		void *next;
	};

	struct SIDSIG
	{
		int *bytes;
		void *next;
	};

	enum { NAME = -4, AND, ANY, END };

	void init(void);
	void readconfig(char *name);
	int identifybufferi(sSIDID *id, unsigned char *buffer, int length);
	int identifybytes(int *bytes, unsigned char *buffer, int length);
	int ishex(char c);
	int gethex(char c);
	void printstats(void);

	sSIDID *firstid;
	sSIDID *lastid;
	sSIDID *playerid;

	bool status;

};

#endif
