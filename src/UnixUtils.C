///
/// @file
/// @ingroup irad_group
/// @brief Unix System tools implementation
///
#include <sstream>
#include <cstdlib>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _WIN32
#include <WinSock2.h>
#endif
#include "UnixUtils.H"

extern char **environ;

namespace IRAD {
  namespace Sys {


	//these permission mode bits mean nothing for windows, as far as I can tell. Windows doesn't use these permissions.
#ifdef _WIN32
    #define S_IRGRP 0
    #define S_IXGRP 0
#endif
	  static const char letters[] =
		  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";


    int Rename(const std::string &source_file,const std::string &target_file)
    {
      return(rename(source_file.c_str(),target_file.c_str()));
    }
	//removed until doing symlinks in Windows C++ is figured out (seems to work now)
   int SymLink(const std::string &source,const std::string &target)
    {
      return(symlink(source.c_str(),target.c_str()));
    }

    const std::string Hostname(bool longname)
    {
      char buf[64];
      gethostname(buf,64);
      return(std::string(buf));
    }
    int Remove(const std::string &fname)
    {
      std::cout << fname << std::endl;
      if(ISDIR(fname)){
        Directory RemoveDir(fname);
        if(RemoveDir.size() != 0){
          for(std::vector<std::string>::iterator it = RemoveDir.begin() ; it != RemoveDir.end(); ++it){
            if(Remove(fname + "/" + *it) < 0)
              return -1;
          }
        }
        std::cout << "removing directory" << std::endl;
        return(rmdir(fname.c_str()));
      }
      else{
        std::cout << "removing file" << std::endl;
        return(unlink(fname.c_str()));
      } 
    }
    std::string LogTime()
    {
      time_t t_ptr;
      time(&t_ptr);
      char timesbuf[64];
      std::string format("[%m/%d/%y%%%X]: ");
      strftime(timesbuf,64,format.c_str(),gmtime(&t_ptr));
      std::string retval(timesbuf);
      return(retval);
    }
    void SafeRemove(const std::string &fname,const std::string &ext)
    {
      if(!FILEEXISTS(fname))
	return;
      if(ISLINK(fname)){
	unlink(fname.c_str());
	return;
      }
      std::string savename_base(fname+"."+ext);
      std::string savename(savename_base);
      unsigned int n = 1;
      while(FILEEXISTS(savename)){
	std::ostringstream Ostr;
	Ostr << savename_base << n++;
	savename.assign(Ostr.str());
      }
      rename(fname.c_str(),savename.c_str());
    }

    bool FILEEXISTS(const std::string &fname)
    {
      struct stat fstat;
	  //changed to stat from lstat because there's no lstat on windows and symlink functionality doesn't work on here yet, and 
	  //symlinks aren't as frequently used on  (THIS HAS BEEN FIXED WITH THE NTLINK LIBRARY)
#ifdef __linux__
      if(lstat(fname.c_str(),&fstat))
		  return false;
#elif _WIN32 
	  if(lstat(fname.c_str(),&fstat))
	      return false;
#endif
	return false;
      return true;
    }

    bool ISDIR(const std::string &fname)
    {
      struct stat fstat;
      if(stat(fname.c_str(),&fstat))
	return false;
      if(S_ISDIR(fstat.st_mode))
	return true;
      return false;
    }
	//removing ISLINK because there's no ISLNK macro in windows, well its not the same as linux and I haven't gotten to figure it out yet
	//maybe something similar is in MinGW's implementation of stat (WORKS NOW)
   bool ISLINK(const std::string &fname)
    {
      struct stat fstat;
      if(lstat(fname.c_str(),&fstat))
	    return false;
      if(S_ISLNK(fstat.st_mode))
	return true;
      return(false);
    }

    int CreateDirectory(const std::string &fname)
    {
#ifdef __linux__
	 return(mkdir(fname.c_str(), S_IRGRP | S_IXGRP | S_IRWXU));
#elif _WIN32
     return (_mkdir(fname.c_str()));
	  
#endif
    }

    const std::string ResolveLink(const std::string &path)
    {
      std::string retVal;
      char buf[1024];
      size_t s = readlink(path.c_str(),buf,1024);
      if(!(s <= 0)){
	buf[s] = '\0';
	retVal.assign(buf);
      }
      std::string::size_type x = retVal.find_last_of("/");
      if(x != std::string::npos)
	retVal.erase(x);
      return (retVal);
    }


    Directory::Directory(const std::string &path)
    {
      _good = false;
      _dir = NULL;
      _path.assign(path);
      if(open(path))
	std::cerr << "Directory::Error: Could not open " << path
		  << " as a directory." << std::endl;
    }

    Directory::~Directory()
    {
      if (_good)
	closedir(_dir);
    }

    Directory::operator void* ()
    {
      return(_good ? this : NULL);
    }

    bool Directory::operator ! ()
    {
      return(!_good);
    }

    void Directory::close()
    {
      if(_good)
	closedir(_dir);
    }

    int Directory::open(const std::string &path)
    {
      if(_good){
	this->close();
	_path = path;
      }
      if(path.empty())
	return(1);
      if(!(_dir = opendir(path.c_str())))
	return(1);
      _path = path;
      _good = true;
      struct dirent *entry;
      // Skip . and ..
      while((entry = readdir(_dir)) != NULL){
        if(std::string(entry->d_name) != "." && std::string(entry->d_name) != "..")
	  this->push_back(entry->d_name);
      }
      return(0);
    }

    const std::string CWD()
    {
      char buf[1024];
      return(std::string(getcwd(buf,1024)));
    }

    int ChDir(const std::string &path)
    {
      return(chdir(path.c_str()));
    }

    const std::string
    StripDirs(const std::string &pname)
    {
      std::string retval;
      std::string::size_type x = pname.find("/");
      if(x == std::string::npos)
	return(pname);
      return(pname.substr(pname.find_last_of("/")+1));
    }

    std::string
    TempFileName(const std::string &stub)
    {
      int fd;
      std::string tstub(stub);
      tstub += "XXXXXX";
      char *name = new char [tstub.length() + 1];
      std::strcpy(name,tstub.c_str());
      name[tstub.length()] = '\0';
      fd = mkstemp(name);
      tstub.assign(name);
      //      if(!fd)
      //	stub = name;
      delete [] name;
      close(fd);
      // Get rid of the file - we just
      // want the name at this point.
      IRAD::Sys::Remove(tstub);
      return (tstub);
    }

    int OpenTemp(std::string &stub)
    {
      int fd;
      std::string tstub(stub);
      tstub += "XXXXXX";
      char *name = new char [tstub.length() + 1];
      std::strcpy(name,tstub.c_str());
      name[tstub.length()] = '\0';
      fd = mkstemp(name);
      stub = name;
      delete [] name;
      return(fd);
    }
    // Tokenize Path
    // Takes an input path and makes string tokens of each
    // directory and the final argument
    void
    TokenizePath(std::vector<std::string> rv,const std::string &path)
    {
      rv.resize(0);
      std::istringstream Istr(path);
      std::string tok;
      while(std::getline(Istr,tok,'/'))
	if(!tok.empty())
	  rv.push_back(tok);
    }

    Environment::Environment()
    {
      this->init();
    }

    void
    Environment::init()
    {
      this->clear();
      std::vector<std::string> raw_env;
      unsigned int i = 0;
      while(environ[i])
	raw_env.push_back(environ[i++]);
      //    Util::Vectorize(raw_env,(const char **)environ);
      std::vector<std::string>::iterator rei = raw_env.begin();
      while(rei != raw_env.end()){
	unsigned int x = (*rei).find("=");
	std::string var((*rei).substr(0,x));
	std::string val((*rei).substr(x+1));
	this->push_back(make_pair(var,val));
	rei++;
      }
    }

    int
    Environment::SetEnv(const std::string &var,const std::string &val,bool ow)
    {
		int retVal = 0;
#ifdef __linux__
		retVal = setenv(var.c_str(),val.c_str(),(int)ow);
#elif _WIN32 
		if (ow) {
            retVal = _putenv_s(var.c_str(), val.c_str());
		}

		else {
			size_t requiredSize; 
			char * retPtr = getenv(var.c_str());
			if (retPtr == NULL) {
				retVal = -1;
			}

			else {
				retVal = 0;
			}
		}
		
#endif
	  
      this->init();
      return(retVal);
    }

    void
    Environment::UnSetEnv(const std::string &var)
    {
		_putenv_s(var.c_str(), "");
      this->init();
    }

#ifndef DARWIN
    int
    Environment::ClearEnv()
    {
		_environ = NULL;
      this->init();
      return(1);
    }
#endif

    const std::string
    Environment::GetEnv(const std::string &var) const
    {
      Environment::const_iterator ei = this->begin();
      while(ei != this->end()){
	if((*ei).first == var)
	  return((*ei).second);
	ei++;
      }
      return(std::string(""));
    }

    std::string &
    Environment::GetEnv(const std::string &var)
    {
      Environment::iterator ei = this->begin();
      while(ei != this->end()){
	if((*ei).first == var)
	  return((*ei).second);
	ei++;
      }
      empty_string.clear();
      return(empty_string);
    }

    int
    Environment::PutEnv(char *envs)
    {
      int retVal = putenv(envs);
      this->init();
      return(retVal);
    }

    void
    Environment::Refresh()
    {
      this->init();
    }

    char **
    Environment::GetRawEnv()
    {
      return(environ);
    }

    std::ostream& operator<<(std::ostream &output,const Environment &env)
    {
      std::vector< std::pair<std::string,std::string> >::const_iterator ei = env.begin();
      while(ei != env.end()){
	output << (*ei).first << " = " << (*ei).second << std::endl;
	ei++;
      }
      return(output);
    }

	/* Generate a temporary file name based on TMPL.  TMPL must match the
	rules for mk[s]temp (i.e. end in "XXXXXX").  The name constructed
	does not exist at the time of the call to mkstemp.  TMPL is
	overwritten with the result.  */
	int mkstemp(char *tmpl)
	{
		int len;
		char *XXXXXX;
		static unsigned long long value;
		unsigned long long random_time_bits;
		unsigned int count;
		int fd = -1;
		int save_errno = errno;

		/* A lower bound on the number of temporary files to attempt to
		generate.  The maximum total number of temporary file names that
		can exist for a given template is 62**6.  It should never be
		necessary to try all these combinations.  Instead if a reasonable
		number of names is tried (we define reasonable as 62**3) fail to
		give the system administrator the chance to remove the problems.  */
#define ATTEMPTS_MIN (62 * 62 * 62)

		/* The number of times to attempt to generate a temporary file.  To
		conform to POSIX, this must be no smaller than TMP_MAX.  */
#if ATTEMPTS_MIN < TMP_MAX
		unsigned int attempts = TMP_MAX;
#else
		unsigned int attempts = ATTEMPTS_MIN;
#endif

		len = strlen(tmpl);
		if (len < 6 || strcmp(&tmpl[len - 6], "XXXXXX"))
		{
			errno = EINVAL;
			return -1;
		}

		/* This is where the Xs start.  */
		XXXXXX = &tmpl[len - 6];

		/* Get some more or less random data.  */
		{
			SYSTEMTIME      stNow;
			FILETIME ftNow;

			// get system time
			GetSystemTime(&stNow);
			stNow.wMilliseconds = 500;
			if (!SystemTimeToFileTime(&stNow, &ftNow))
			{
				errno = -1;
				return -1;
			}

			random_time_bits = (((unsigned long long)ftNow.dwHighDateTime << 32)
				| (unsigned long long)ftNow.dwLowDateTime);
		}
		value += random_time_bits ^ (unsigned long long)GetCurrentThreadId();

		for (count = 0; count < attempts; value += 7777, ++count)
		{
			unsigned long long v = value;

			/* Fill in the random bits.  */
			XXXXXX[0] = letters[v % 62];
			v /= 62;
			XXXXXX[1] = letters[v % 62];
			v /= 62;
			XXXXXX[2] = letters[v % 62];
			v /= 62;
			XXXXXX[3] = letters[v % 62];
			v /= 62;
			XXXXXX[4] = letters[v % 62];
			v /= 62;
			XXXXXX[5] = letters[v % 62];

			fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, _S_IREAD | _S_IWRITE);
			if (fd >= 0)
			{
				errno = save_errno;
				return fd;
			}
			else if (errno != EEXIST)
				return -1;
		}

		/* We got out of the loop because we ran out of combinations to try.  */
		errno = EEXIST;
		return -1;
	}

  };
};

