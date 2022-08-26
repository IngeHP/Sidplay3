#include "sidid.h"
#include "windows.h"
#include "stdio.h"
#include "stdlib.h"


SIDID::SIDID()
{
	const char *filename = "sidid.cfg";
	char configfilename[MAX_PATH];
	int l;
	l = GetModuleFileNameA(NULL, configfilename, MAX_PATH);
	while ((configfilename[l-1] != '\\') && (l > 1))
		l--;
	if(l > 1)
	{
		for (int i = 0; i < (int)strlen(filename); i++)
		{
			configfilename[l++] = filename[i];
		}
		configfilename[l] = '\0';
		readconfig(configfilename);
	}
	else
		status = false;

	playersfound[0] = 0x00;

}

SIDID::SIDID(char *configname)
{
	readconfig(configname);
	playersfound[0] = 0x00;
}

SIDID::~SIDID()
{
	SIDSIG *sig, *tempsig;
	sSIDID *tempid;

	while (firstid)
	{
		if (firstid->firstsig)
		{
			sig = (SIDSIG *)firstid->firstsig;
			while (sig->next)
			{
				tempsig = (SIDSIG *)sig->next;
				delete sig;
				sig = tempsig;
			}
			delete sig;
		}

		tempid = (sSIDID *)firstid->next;
		delete firstid;
		firstid = tempid;

	}
	if (firstid)
		delete firstid;
}

void SIDID::readconfig(char *name)
{
	status = false;
	char tokenstr[MAX_PATH];
	int temp[MAX_SIGSIZE];
	int sigsize = 0;
	SIDSIG *lastsig = NULL;

	char playername[MAX_PATH];

	firstid = NULL;
	lastid = NULL;
	playerid = NULL;

	playername[0] = 0;
	FILE *in;
	fopen_s(&in, name, "rt");

	if (!in) return;

	for (;;)
	{
		int len;

		tokenstr[0] = 0;
		fscanf_s(in, "%s", tokenstr, _countof(tokenstr));
		len = strlen(tokenstr);

		if (len)
		{
		int token = NAME;

		if (!strcmp("??", tokenstr)) token = ANY;
		if ((!strcmp("end", tokenstr)) || (!strcmp("END", tokenstr))) token = END;
		if ((!strcmp("and", tokenstr)) || (!strcmp("AND", tokenstr))) token = AND;
		if ((len == 2) && (ishex(tokenstr[0])) && (ishex(tokenstr[1])))
		{
			token = gethex(tokenstr[0]) * 16 + gethex(tokenstr[1]);
		}

      switch (token)
      {
        case NAME:
        {
          sSIDID *newid = new sSIDID;
          if (!newid)
          {
          	status = false;
          	goto CONFIG_ERROR;
          }
          newid->name = _strdup(tokenstr);
          newid->firstsig = NULL;
          newid->next = NULL;
          newid->count = 0;

          if (!strcmp(playername, newid->name)) playerid = newid;

          if (!firstid)
          {
          	firstid = newid;
          }
          else
          {
            if (lastid) lastid->next = (void *)newid;
          }
          lastid = newid;

          sigsize = 0;
        }
        break;

        case END:
        if (sigsize >= MAX_SIGSIZE)
        {
          //printf("Maximum signature size exceeded!\n");
			status = false;
          goto CONFIG_ERROR;
        }
        else
        {
          temp[sigsize++] = END;
          if (sigsize > 1)
          {
            int c;

            SIDSIG *newsig = new SIDSIG;
            int *newbytes = new int[sigsize];
            if ((!newsig) || (!newbytes))
            {
              //printf("Out of memory!\n");
				status = false;
          	  goto CONFIG_ERROR;
            }
            newsig->bytes = newbytes;
            newsig->next = NULL;
            for (c = 0; c < sigsize; c++)
            {
              newsig->bytes[c] = temp[c];
            }

            if (!lastid)
          	{

              //printf("No playername defined before signature!\n");
				status = false;
              goto CONFIG_ERROR;
          	}
            else
            {
              if (!lastid->firstsig)
              {
              	lastid->firstsig = (void *)newsig;
              }
              else
              {
                if (lastsig)
                {
            	    lastsig->next = (void *)newsig;
                }
              }
            }
            lastsig = newsig;
          }
        }
        sigsize = 0;
        break;

        default:
        if (sigsize >= MAX_SIGSIZE)
        {
			//printf("Maximum signature size exceeded!\n");
			status = false;
			goto CONFIG_ERROR;
        }
        temp[sigsize++] = token;
        break;
      }
    }
    else break;
  }
  status = true;
  CONFIG_ERROR:
  fclose(in);
}

void SIDID::init()
{
}

int SIDID::ishex(char c)
{
  if ((c >= '0') && (c <= '9')) return 1;
  if ((c >= 'a') && (c <= 'f')) return 1;
  if ((c >= 'A') && (c <= 'F')) return 1;
  return 0;
}

int SIDID::gethex(char c)
{
  if ((c >= '0') && (c <= '9')) return c - '0';
  if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
  if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
  return -1;
}

int SIDID::identifybuffer(unsigned char *buffer, int length)
{
	strcpy_s(playersfound,MAX_SIGSIZE,"Unknown\0");
	sSIDID *id;
	int found = 0;
	id = firstid;

	while (id)
	{
		if(identifybufferi(id,buffer,length))
		{
			id->count++;
			if (!found)
			{
				strcpy_s(playersfound,MAX_SIGSIZE,id->name);
			}
			else
			{
				if ((strncmp(id->name,"(",1))||(strncmp(id->name,"+",1)))
					strcat_s(playersfound,"+");
				strcat_s(playersfound,id->name);
			}
			found++;
		}
		id = (sSIDID *)id->next;
	}

	return found;
}			

int SIDID::identifybufferi(sSIDID *id, unsigned char *buffer, int length)
{
  SIDSIG *sig = (SIDSIG*)id->firstsig;

  while (sig)
  {
    if (identifybytes(sig->bytes, buffer, length)) return 1;
    sig = (SIDSIG *)sig->next;
  }
  return 0;
}

int SIDID::identifybytes(int *bytes, unsigned char *buffer, int length)
{
  int c = 0, d = 0, rc = 0, rd = 0;

  while (c < length)
  {
    if (d == rd)
    {
      if (buffer[c] == bytes[d])
      {
        rc = c+1;
        d++;
      }
      c++;
    }
    else
    {
  		if (bytes[d] == END) return 1;
  		if (bytes[d] == AND)
  		{
        d++;
        while (c < length)
        {
          if (buffer[c] == bytes[d])
          {
            rc = c+1;
            rd = d;
            break;
          }
          c++;
        }
        if (c >= length)
          return 0;
  		}
      if ((bytes[d] != ANY) && (buffer[c] != bytes[d]))
      {
        c = rc;
        d = rd;
      }
      else
      {
        c++;
        d++;
      }
    }
  }
  return 0;
}


int SIDID::identifyfile(char *name)
{
  unsigned char *buffer = NULL;
  sSIDID *id;
  int length;
  int found = 0;
  strcpy_s(playersfound,MAX_SIGSIZE,"Unknown\0");

  id = firstid;
 

  FILE *in;
  if ((fopen_s(&in, name, "rb")) != 0) return -1;

  fseek(in, 0, SEEK_END);
  length = ftell(in);
  fseek(in, 0, SEEK_SET);
  buffer = new unsigned char[length];
  

  if (!buffer)
  {
    fclose(in);
    return -1;
  }

  fread(buffer, 1, length, in);
  fclose(in);

	while (id)
	{
		if(identifybufferi(id,buffer,length))
		{
			id->count++;
			if (!found)
			{
				strcpy_s(playersfound,MAX_SIGSIZE,id->name);
			}
			else
			{
				if (strncmp(id->name,"(",1))
					strcat_s(playersfound,"+");
				strcat_s(playersfound,id->name);
			}
			found++;
		}
		id = (sSIDID *)id->next;
	}
	delete buffer;
	return found;
	

}

