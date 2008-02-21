#include "chunk.h"

/* Initialize struct CHUNK */
CHUNK *chunk_init()
{
	CHUNK *chunk = (CHUNK *)calloc(1, sizeof(CHUNK));
	if(chunk)
	{
		chunk->set	= chk_set;
		chunk->append	= chk_append;
		chunk->fill	= chk_fill;
		chunk->send	= chk_send;
		chunk->reset	= chk_reset;
		chunk->clean	= chk_clean;
		chunk->buf	= buffer_init();
#ifdef HAVE_PTHREAD
	chunk->mutex	= calloc(1, sizeof(pthread_mutex_t));
        if(chunk->mutex) pthread_mutex_init((pthread_mutex_t *)chunk->mutex, NULL);
#endif
	}
	return chunk;
}

/* Initialzie CHUNK */
int chk_set(CHUNK *chunk, int id, int type, char *filename, uint64_t offset, uint64_t len)
{
	if(chunk)
	{	
		chunk->reset(chunk);
#ifdef HAVE_PTHREAD
        if(chunk->mutex) pthread_mutex_lock((pthread_mutex_t *)chunk->mutex);
#endif

		chunk->id = id;
		chunk->type = type;
		if(chunk->buf == NULL)
		{
			chunk->buf = buffer_init();
		}
		else
		{
			chunk->buf->reset(chunk->buf);	
		}
		chunk->file.fd = -1;
		if(filename)strcpy(chunk->file.name, filename);
		chunk->offset = offset ;
		chunk->len = len;
#ifdef HAVE_PTHREAD
        if(chunk->mutex) pthread_mutex_unlock((pthread_mutex_t *)chunk->mutex);
#endif

	}
	return 0;	
}

/* append data to CHUNK BUFFER */
int chk_append(CHUNK *chunk, void *data, size_t len)
{
	int ret = -1;
	if(chunk)
	{
#ifdef HAVE_PTHREAD
		if(chunk->mutex) pthread_mutex_lock((pthread_mutex_t *)chunk->mutex);
#endif
		if(chunk->buf && chunk->buf->push(chunk->buf, data, len) == 0 )
		{
			chunk->len += len;
			ret = 0;
		}
#ifdef HAVE_PTHREAD
		if(chunk->mutex) pthread_mutex_unlock((pthread_mutex_t *)chunk->mutex);
#endif

	}
	return ret;
}

#ifndef CLOSE_FD
#define CLOSE_FD(_fd){if(_fd > 0 ){close(_fd);} _fd = -1;}
#endif

/* fill CHUNK with data */
int chk_fill(CHUNK *chunk, void *data, size_t len)
{
	int n = 0;
	size_t size;
	if(chunk == NULL ) return -1;
	if(chunk->len <= 0 ) return 0;
#ifdef HAVE_PTHREAD
	if(chunk->mutex) pthread_mutex_lock((pthread_mutex_t *)chunk->mutex);
#endif
	switch(chunk->type)	
	{
		case MEM_CHUNK :
			{
				size = (chunk->len > len)? len : chunk->len;
				if( (n = chunk->buf->push(chunk->buf, data , (size_t)size)) != 0 )
				{	
					n = -1;
				}
				else
				{
					chunk->len  -= size * 1llu;
					n = (int)size;
				}
				break;
			}	
		case FILE_CHUNK :
			{
				if( chunk->file.fd < 0 
						|| (chunk->file.fd = open(chunk->file.name,
								O_CREAT |O_RDWR, 0644)) < 0 )
				{
					n = -1;
					break;
				}
				if(lseek(chunk->file.fd, chunk->offset, SEEK_SET) == -1)
				{
					n = -1;
					CLOSE_FD(chunk->file.fd);
					break;
				}
				size = (chunk->len > len)? len : chunk->len;
				if((n = write(chunk->file.fd, data, size) ) > 0 )
				{
					chunk->offset += n * 1llu;
					chunk->len  -= n * 1llu;
				}
				CLOSE_FD(chunk->file.fd);
				break;
			}
		default :
			n = -1;
	}
#ifdef HAVE_PTHREAD
	if(chunk->mutex) pthread_mutex_unlock((pthread_mutex_t *)chunk->mutex);
#endif
	return n;
}	

/* write CHUNK data to fd */
int chk_send(CHUNK *chunk, int fd, size_t buf_size)
{
	int n = 0, len = 0;
	size_t m_size;
	void *data = NULL;
	void *buf = NULL;

	if(chunk == NULL ) return -1;
	if(chunk->len <= 0 ) return -1;
#ifdef HAVE_PTHREAD
        if(chunk->mutex) pthread_mutex_lock((pthread_mutex_t *)chunk->mutex);
#endif
	switch(chunk->type)	
	{
		case MEM_CHUNK :
			{
				if( (n = write(fd, ((char *)(chunk->buf->data) + chunk->offset),
								chunk->len)) < 0 )
				{	
					n = -1;
				}
				else
				{
					chunk->offset  += n * 1llu;
					chunk->len  -= n * 1llu;
				}
				break;
			}	
		case FILE_CHUNK :
			{
				if(chunk->file.fd < 0 )
				{
					if( (chunk->file.fd = open(chunk->file.name, O_RDONLY)) < 0 )
					{
						n = -1;
						goto end;
					}
				}
				if(lseek(chunk->file.fd, (off_t) chunk->offset, SEEK_SET) == -1)
                                {
                                        n = -1;
                                        fprintf(stderr, "LSEEK %u failed, %s\n",
                                                         chunk->offset, strerror(errno));
                                        goto end;
                                }
#ifdef _USE_MMAP
				m_size = (chunk->len > buf_size)
					 ? buf_size : (size_t)(chunk->len);
				if( (data = mmap(NULL, m_size, PROT_READ, MAP_PRIVATE,
						chunk->file.fd, 0)) == MAP_FAILED)
				{
					n = -1;
					fprintf(stderr, "MMAP %d size:%u failed, %s\n",
							chunk->file.fd, m_size, strerror(errno));
					goto end;
				}
#else
				data = buf = (void *)calloc(1, buf_size);
				if(( len = read(chunk->file.fd, data, buf_size)) < 0 )
				{
					n = -1;
					fprintf(stderr, "READ %d failed, %s\n", 
						chunk->file.fd, strerror(errno));
					goto end;
				}
				else
				{
					m_size = len;
				}
#endif
				if((n = write(fd, data, m_size) ) < 0 )
				{
					n = -1;
					fprintf(stderr, "WRITE %d failed, %s\n",
                                                chunk->file.fd, strerror(errno));
					goto end;
				}
				else
				{
					chunk->offset += n * 1llu;
					chunk->len  -= n * 1llu;
				}
#ifdef _USE_MMAP
					munmap(data, m_size);
#endif
end:
				{
					if(buf) free(buf);
					CLOSE_FD(chunk->file.fd);
				}
				break;
			}
		default :
			return n = -1;
	}
#ifdef HAVE_PTHREAD
                if(chunk->mutex) pthread_mutex_unlock((pthread_mutex_t *)chunk->mutex);
#endif

	return n;
}

/* reset CHUNK */
void chk_reset(CHUNK *chunk)
{
	if(chunk)
	{
#ifdef HAVE_PTHREAD
		if(chunk->mutex) pthread_mutex_lock((pthread_mutex_t *)chunk->mutex);
#endif
		switch(chunk->type)
		{
			case MEM_CHUNK :
				{
					chunk->buf->reset(chunk->buf);
				}
			case FILE_CHUNK :
				{
					CLOSE_FD(chunk->file.fd);
					break;
				}
			default :
				break;

		}
		chunk->id = 0;
		chunk->type = 0;
		chunk->offset = 0llu;
		chunk->len = 0llu;
#ifdef HAVE_PTHREAD
		if(chunk->mutex) pthread_mutex_unlock((pthread_mutex_t *)chunk->mutex);
#endif

	}
}

/* clean CHUNK buffer data and close opened fd */
void chk_clean(CHUNK **chunk)
{
	if((*chunk) == NULL) return ;
	//chunk->reset(chunk);
        if((*chunk)->buf)
        {
                (*chunk)->buf->clean(&(*chunk)->buf);
        }
#ifdef HAVE_PTHREAD
	if((*chunk)->mutex)
	{
		pthread_mutex_unlock((pthread_mutex_t *)(*chunk)->mutex);
		pthread_mutex_destroy((pthread_mutex_t *)(*chunk)->mutex);
		free((*chunk)->mutex);
	}	
#endif
        free((*chunk));
        (*chunk) = NULL;
        return ;
}

#ifdef _DEBUG_CHUNK
int main()
{
	CHUNK *chunk = chunk_init();
	char *s = "d,f.ma.sdfmds.fm;ldsmf.ds,f.ds,f/.df";
	if(chunk)
	{
		chunk->set(chunk, 0, MEM_CHUNK, NULL, 0, 160);
		CHUNK_VIEW(chunk);
		chunk->fill(chunk, (void *)s, strlen(s));
		CHUNK_VIEW(chunk);
		chunk->send(chunk, 0, 32);
		chunk->clean(&chunk);
	}		
}	
#endif
