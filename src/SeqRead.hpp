#include <stdlib.h>

#define READLENGTH 8*1024

class SeqRead
{
public:
    SeqRead(char *path, uint64_t offset, uint64_t length);
    ~SeqRead();
    void init();
    bool getRead();

    bool m_isEof;
    char m_lastchar;
    FILE *m_f;
    char *m_pbuffer;
    int index;
    int m_step;
    uint64_t m_length;
    int m_begin,m_end;
    char name[1024];
    char seq[1024];
    char name2[1024];
    char qual[1024];
};

SeqRead::SeqRead(char *path, uint64_t offset, uint64_t length)
{
    m_isEof = false;
    m_begin = 0;
    m_end = 0;
    m_f = fopen(path,"r");
    fseek(m_f, offset, SEEK_SET);
    m_length = length; //单个slice的长度
    m_pbuffer = (char*)malloc(READLENGTH);
}

SeqRead::~SeqRead()
{
    free(m_pbuffer);
    fclose(m_f);
}

void SeqRead::init()
{
    memset(name, 0, 1024);
    memset(seq, 0, 1024);
    memset(name2, 0, 1024);
    memset(qual, 0, 1024);
}

bool SeqRead::getRead()
{
    init();
    index = 0;
    m_step = 1;

READ:
    if(m_begin >= m_end-1)
    {
        if(m_isEof)
        {
            return strlen(qual);
        }

        if(m_lastchar == '\n')//判断如何拼接
        {
            m_step++;
            index=0;
        }

        m_begin = 0;
        int len_read = m_length < READLENGTH ? m_length : READLENGTH;
        //m_end = gzread(m_f, m_pbuffer,len_read);
        m_end = fread(m_pbuffer, 1, len_read, m_f);
        m_length -= m_end;
        m_lastchar = m_pbuffer[m_end-1];
        if(m_end < READLENGTH) //到达结尾
        {
            //printf("%d\n", m_length);
            m_isEof = true;
        }
    }
    
    if(m_step == 1)
    {
        if(m_pbuffer[m_begin] == '@') m_begin++;
        while(m_pbuffer[m_begin] != '\n') //get name
        {
            if(m_begin < m_end)
            {
                name[index++] = m_pbuffer[m_begin++];
            }
            else
            {
                goto READ;
            }
        }
        if(m_begin >= m_end-1)
        {
            goto READ;
        }
        m_step++;
        index=0;
        m_begin++;
    }

    if(m_step == 2)
    {
        while(m_pbuffer[m_begin] != '\n') //get name
        {
            if(m_begin < m_end)
            {
                seq[index++] = m_pbuffer[m_begin++];
            }
            else
            {
                goto READ;
            }
        }
        if(m_begin >= m_end-1)
        {
            goto READ;
        }
        m_step++;
        index=0;
        m_begin++;
    }

    if(m_step == 3)
    {
        while(m_pbuffer[m_begin] != '\n') //get name
        {
            if(m_begin < m_end)
            {
                name2[index++] = m_pbuffer[m_begin++];
            }
            else
            {
                goto READ;
            }
        }
        if(m_begin >= m_end-1)
        {
            goto READ;
        }
        m_step++;
        index=0;
        m_begin++;
    }

    if(m_step == 4)
    {
        while(m_pbuffer[m_begin] != '\n') //get name
        {
            if(m_begin < m_end)
            {
                qual[index++] = m_pbuffer[m_begin++];
            }
            else
            {
                goto READ;
            }
        }
        if(m_begin >= m_end-1)
        {
            goto READ;
        }
        m_begin++;
    }

    if(strlen(seq) == strlen(qual))
    {
        return true;
    }
    else
    {
        printf("%s  not equal\n", name);
    }

    return false;
}