/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_FILE_H
#define HZ_FS_FILE_H

#include "hz_config.h"  // feature macros

#include <string>
#include <stdio.h>  // (not cstdio!) off_t, FILE, fopen and friends
#include <sys/types.h>  // stat() needs this;
#include <sys/stat.h>  // stat() needs this; chmod()

#ifdef _WIN32
	#include <io.h>  // stat(), unlink()
#else
	#include <unistd.h>  // stat(), unlink()
#endif

#include "fs_path.h"  // Path


// Filesystem file manipulation


namespace hz {



// Offset & size type. may be uint32_t or uint64_t, depending on system and compilation flags.
// Note: It is usually discouraged to use this type in headers, because the library may be
// compiled with one size and the application with another. However, this problem is rather
// limited if using header-only approach, as we do.
#ifndef _WIN32
	typedef off_t file_size_t;  // off_t is in stdio.h
#else  // win32 doesn't have off_t
	typedef long file_size_t;  // fseek() and friends use long.
#endif


/*
TODO

Seekless read support in hz::File. should be able to accept a fixed buffer,
an std::string of max length, optional ticker callback.
e.g.

// callback
inline bool append_func(std::string& put_here, const std::string& chunk, bool last)
{
	put_here += chunk;
	return true;  // continue
}

// user code
void f (hz::File& file)
{
	std::string put_here;
	file.get_contents(put_here, 1024, append_func);  // 3rd param should have a default func.
}

Should have similar thing with static buffers.
*/



class File : public FsPath {

	public:
		typedef FILE* handle_type;

		File() : file_(NULL)
		{ }

		// Create a File object with path "path". This will NOT open the file.
		File(const std::string& path) : file_(NULL)
		{
			set_path(path);
		}

		// Create a File object with path "path". This will NOT open the file.
		File(const FsPath& path) : file_(NULL)
		{
			set_path(path.get_path());
		}

		// Create a File object and open a file "path" points to.
		// You should check the success status with bad().
		File(const std::string& path, const std::string& open_mode) : file_(NULL)
		{
			set_path(path);
			open(open_mode);
		}

		// Create a File object and open a file "path" points to.
		// You should check the success status with bad().
		File(const FsPath& path, const std::string& open_mode) : file_(NULL)
		{
			set_path(path.get_path());
			open(open_mode);
		}


	private:
		// Between move semantics (confusing and error-prone) and denying copying,
		// I choose to deny.

		File(const File& other);  // copy constructor. needed to override File(const FsPath&).

		File& operator= (const File& other);


	public:

/*
		// Copy constructor. Move semantics are implemented - the ownership is transferred
		// exclusively.
		File(File& other) : file_(NULL)
		{
			*this = other;  // operator=
		}

		// move semantics, as with copy constructor.
		inline File& operator= (File& other);
*/


		// Destructor which invokes close() if needed.
		~File()
		{
			if (file_)
				close();
		}


		// --- these may set bad() status


		// Open the file with open_mode.
		inline bool open(const std::string& open_mode);

		// Open the previously opened file.
		inline bool close();

		// Get native file handle (obtained using fopen())
		handle_type get_handle()
		{
			return file_;
		}


		// Returns false on error. You must call "delete[] put_data_here".
		// If the file is larger than 100M (by default), the function refuses to load it.
		// Note: No additional trailing 0 is written to data!
		inline bool get_contents(unsigned char*& put_data_here,
				file_size_t& put_size_here, file_size_t max_size = 104857600);

		// Same as above, but puts data into already allocated buffer.
		// If the buffer is of insufficient size, false is returned and buffer is left untouched.
		// If any other error occurs, the buffer may be left in unspecified state.
		inline bool get_contents_noalloc(unsigned char*& put_data_here, file_size_t buf_size,
				file_size_t& put_size_here, file_size_t max_size = 104857600);

		// same as above, but for std::string (no terminating 0 is needed inside the file).
		inline bool get_contents(std::string& put_data_here, file_size_t max_size = 104857600);

		// write data to file, creating or truncating it beforehand. (no terminating 0 is needed inside data).
		inline bool put_contents(const unsigned char* data, file_size_t data_size);

		// same as above, for std::string (no terminating 0 is needed inside data or anywhere else)
		inline bool put_contents(const std::string& data);


		// Do NOT assign the result to int - you'll break LFS support.
		inline bool get_size(file_size_t& put_here, bool use_read = false);


		// Move (rename) a file to "to". The destination will be overwritten (if it exists and
		// it's not a directory (even in win32)). This function is subject to rename() limitations.
		// On error it sets errors in both the returned and current objects. returns the new path.
		inline bool move(const std::string& to);

		// Copy one file to a "to" destination (with destination being a filename).
		// If "to" already exists, overwrite it. Return the new path.
		// On error the errors are set in both the returned and current objects.
		inline bool copy(const std::string& to);

		// remove() is in Path (parent).


	private:

		// for move semantics:

// 		File(const File& other);  // don't allow it. allow only from non-const.

// 		const File& operator=(const File& other);  // don't allow it. allow only from non-const.


		handle_type file_;  // FILE*

};





// ------------------------------------------- Implementation


/*
// move semantics, as with copy constructor.
inline File& File::operator= (File& other)
{
	file_ = other.file_;

	// clear other's members, so everything is clear.
	other.file_ = NULL;
	other.set_path("");
	other.set_error(HZ__("The file handle ownership was transferred from this object."));

	return *this;
}
*/


// Open the file with open_mode.
inline bool File::open(const std::string& open_mode)
{
	clear_error();

	if (file_) {  // already open
		set_error(std::string(HZ__("Error while opening file \"/path1/\": "))
				+ HZ__("Another file is open already. Close it first."), 0, path_);
		return false;
	}
	if (path_.empty()) {
		set_error(std::string(HZ__("Error while opening file: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	file_ = fopen(path_.c_str(), open_mode.c_str());
	if (!file_) {
		set_error(HZ__("Error while opening file \"/path1/\": /errno/."), errno, path_);
		return false;
	}

	return true;
}


// Open the previously opened file.
inline bool File::close()
{
	clear_error();

	if (file_ && fclose(file_) != 0) {  // error
		set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, path_);
		file_ = NULL;
		return false;
	}

	file_ = NULL;
	return true;  // even if already closed
}



// Returns false on error. You must call "delete[] put_data_here".
// If the file is larger than 100M (by default), the function refuses to load it.
// Note: No additional trailing 0 is written to data!
inline bool File::get_contents(unsigned char*& put_data_here,
		file_size_t& put_size_here, file_size_t max_size)
{
	return get_contents_noalloc(put_data_here, static_cast<file_size_t>(-1), put_size_here, max_size);
}



// Same as above, but puts data into already allocated buffer.
// If the buffer is of insufficient size, false is returned and buffer is left untouched.
// If any other error occurs, the buffer may be left in unspecified state.
// Internal usage only: If buf_size is -1, the buffer will be automatically allocated.
// FIXME: Support files which can't be seeked and don't have a size attribute (e.g. /proc/*).
inline bool File::get_contents_noalloc(unsigned char*& put_data_here, file_size_t buf_size,
		file_size_t& put_size_here, file_size_t max_size)
{
	clear_error();

	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to open file for reading: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	FILE* f = fopen(path_.c_str(), "rb");
	if (!f) {
		set_error(HZ__("Unable to open file \"/path1/\" for reading: /errno/."), errno, path_);
		return false;
	}


	do {  // goto emulation

		if (fseek(f, 0, SEEK_END) != 0) {  // this should work in LFS too (I think)
			set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, path_);
			break;  // goto cleanup
		}

#ifndef _WIN32  // win32 doesn't have ftello()
		const file_size_t size = ftello(f);  // LFS variant of ftell()
#else
		const file_size_t size = ftell(f);  // aka long
#endif
		if (size == static_cast<file_size_t>(-1)) {  // size may be unsigned, it's the way it works
			set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, path_);
			break;  // goto cleanup
		}

		if (size > max_size) {
			set_error(std::string(HZ__("Unable to read file \"/path1/\": ")) + HZ__("File size is larger than allowed."), 0, path_);
			break;  // goto cleanup
		}

		// automatically allocate the buffer if buf_size is -1.
		bool auto_alloc = (buf_size == static_cast<file_size_t>(-1));
		if (!auto_alloc && buf_size < size) {
			set_error(std::string(HZ__("Unable to read file \"/path1/\": ")) + HZ__("Supplied buffer is too small."), 0, path_);
			break;  // goto cleanup
		}

		rewind(f);  // returns void

		unsigned char* buf = put_data_here;
		if (auto_alloc)
			buf = new unsigned char[static_cast<unsigned int>(size)];  // this may throw!

		// FIXME: Large file support
		size_t read_bytes = fread(buf, 1, size, f);  // I really hope this is not a byte-by-byte operation
		if (size != static_cast<file_size_t>(read_bytes)) {
			set_error(std::string(HZ__("Unable to read file \"/path1/\": "))
					+ HZ__("Unexpected number of bytes read."), 0, path_);
			if (auto_alloc)
				delete[] buf;
			break;  // goto cleanup
		}

		// All OK
		put_size_here = size;
		if (auto_alloc)
			put_data_here = buf;

	} while (false);


	// cleanup:
	if (fclose(f) != 0) {
		if (!bad())  // don't overwrite the previous error
			set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, path_);
	}

	return ok();
}



// same as above, but for std::string (no terminating 0 is needed inside the file).
inline bool File::get_contents(std::string& put_data_here, file_size_t max_size)
{
	file_size_t size = 0;
	unsigned char* buf = 0;
	get_contents(buf, size, max_size);
	if (bad())
		return false;

	// Note: No need for appending 0, it's all automatic in string.
	put_data_here.reserve(static_cast<std::string::size_type>(size));  // string takes size without trailing 0.
	put_data_here.append(reinterpret_cast<char*>(buf), static_cast<std::string::size_type>(size));

	delete[] buf;
	return ok();
}



// write data to file, creating or truncating it beforehand. (no terminating 0 is needed inside data).
inline bool File::put_contents(const unsigned char* data, file_size_t data_size)
{
	clear_error();

	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to open file for for writing: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	FILE* f = fopen(path_.c_str(), "wb");
	if (!f) {
		set_error(HZ__("Unable to open file \"/path1/\" for writing: /errno/."), errno, path_);
		return false;
	}

	// We write in chunks to support large files.
	const size_t chunk_size = 32*1024;  // 32K block devices will be happy.
	file_size_t left_to_write = data_size;

	bool write_error = false;
	while (left_to_write >= static_cast<file_size_t>(chunk_size)) {
		// better loop than specify all in one, this way we can support _really_ large files.
		if (fwrite(data + data_size - left_to_write, chunk_size, 1, f) != 1) {
			write_error = true;
			break;
		}
		left_to_write -= chunk_size;
	}

	// write the remainder
	if (!write_error && fwrite(data + data_size - left_to_write, left_to_write, 1, f) != 1)
		write_error = true;

	if (write_error) {
		set_error(std::string(HZ__("Unable to write file \"/path1/\": "))
				+ HZ__("Number of written bytes doesn't match the data size."), 0, path_);
		fclose(f);  // don't check anything, it's too late
		return false;
	}

	if (fclose(f) != 0)
		set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, path_);

	return ok();
}


// same as above, for std::string (no terminating 0 is needed inside data or anywhere else)
inline bool File::put_contents(const std::string& data)
{
	return put_contents(reinterpret_cast<const unsigned char*>(data.data()), data.size());
}




// Do NOT assign the result to int - you'll break LFS support.
// use_read parameter forces actual reading of a file, useful for files in /proc.
inline bool File::get_size(file_size_t& put_here, bool use_read)
{
	clear_error();

	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to get file size: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	if (!use_read) {
		struct stat s;
		if (stat(path_.c_str(), &s) == -1) {
			set_error(HZ__("Unable to get file size of \"/path1/\": /errno/."), errno, path_);
			return false;
		}

		if (!S_ISREG(s.st_mode)) {  // no need for S_ISLNK - we're using stat(), not lstat().
			set_error("Unable to get file size of \"/path1/\": Supplied path is not a regular file.", 0, path_);
			return false;
		}

		// For symlinks st_size is a size in bytes of the pointed file.
		// To get the size of a symlink itself, use lstat(). Size of a symlink
		// is basically strlen(pointed_path).
		put_here = s.st_size;

		return true;
	}


	FILE* f = fopen(path_.c_str(), "rb");
	if (!f) {
		set_error(HZ__("Unable to open file \"/path1/\" for reading: /errno/."), errno, path_);
		return false;
	}


	const size_t buf_size = 32*1024;  // 32K block devices will be happy
	char buf[buf_size];

	// read until the end
	while (fread(buf, buf_size, 1, f) == 1) {
		// nothing
	}

	if (ferror(f)) {
		set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, path_);

	} else {  // file read completely

#ifndef _WIN32  // win32 doesn't have ftello()
		const file_size_t size = ftello(f);  // LFS variant of ftell()
#else
		const file_size_t size = ftell(f);  // aka long
#endif
		if (size == static_cast<file_size_t>(-1)) {  // size may be unsigned, it's the way it works
			set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, path_);

		} else {
			// All OK
			put_here = size;
		}
	}

	// cleanup:
	if (fclose(f) != 0) {
		if (!bad())  // don't overwrite the previous error
			set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, path_);
	}

	return ok();
}



// Move (rename) a file to "to". The destination will be overwritten (if it exists and
// it's not a directory (even in win32)). This function is subject to rename() limitations.
// On error it sets errors in both the returned and current objects. returns the new path.
inline bool File::move(const std::string& to)
{
	clear_error();

	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to move filesystem entry: ")) + HZ__("Source path is empty."));
		return false;
	}
	if (to.empty()) {
		set_error(std::string(HZ__("Unable to move filesystem entry \"/path1/\": "))
				+ HZ__("Destination path is empty."), 0, path_);
		return false;
	}
	if (path_ == to) {
		set_error(std::string(HZ__("Unable to move filesystem entry \"/path1/\": "))
				+ HZ__("Source path is the same as destination path."), 0, path_);
		return false;
	}

	// win32 rename() doesn't replace contents if newpath exists and says "file exists".
#ifdef _WIN32
/// FIXME: Try to rename first, unlink/rename again later. This way we have at least some atomicity.
	// try remove it if it's a file. no error checks here - rename() will report the needed messages.
	File dest_file(to);
	if (dest_file.is_file())
		dest_file.remove();
#endif

	if (rename(path_.c_str(), to.c_str()) != 0)
		set_error(HZ__("Unable to move filesystem entry \"/path1/\" to \"/path2/\": /errno/."), errno, path_, to);

	return ok();
}




// Copy one file to a "to" destination (with destination being a filename).
// If "to" already exists, overwrite it. Return the new path.
// On error the errors are set in both the returned and current objects.
inline bool File::copy(const std::string& to)
{
	clear_error();

	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to copy file: ")) + HZ__("Source path is empty."));
		return false;
	}
	if (to.empty()) {
		set_error(std::string(HZ__("Unable to copy file \"/path1/\": "))
				+ HZ__("Destination path is empty."), 0, path_);
		return false;
	}
	if (path_ == to) {
		set_error(std::string(HZ__("Unable to copy file \"/path1/\": "))
				+ HZ__("Source path is the same as destination path."), 0, path_);
		return false;
	}

	// remember permissions in case the file is deleted while copying
	struct stat st;
	int stat_result = stat(path_.c_str(), &st);


	FILE* fsrc = fopen(path_.c_str(), "rb");
	if (!fsrc) {
		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Unable to open source file: /errno/."), errno, path_, to);
		return false;
	}

	FILE* fdest = fopen(to.c_str(), "wb");  // this truncates it
	if (!fdest) {
		fclose(fsrc);  // don't check status - there's nothing we can do anyway
		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Unable to create destination file: /errno/."), errno, path_, to);
		return false;
	}


	const size_t buf_size = 32*1024;  // 32K
	unsigned char buf[buf_size] = {0};

	while (true) {
		// let's hope these are not byte-by-byte operations
		size_t read_bytes = fread(buf, 1, buf_size, fsrc);

		if (read_bytes != buf_size && ferror(fsrc)) {  // error
			set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
					+ HZ__("Error while reading source file: /errno/."), errno, path_, to);
			break;
		}

		size_t written_bytes = fwrite(buf, 1, read_bytes, fdest);
		if (read_bytes != written_bytes) {  // error
			set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
					+ HZ__("Error while writing to destination file: /errno/."), errno, path_, to);
			break;
		}
	}

	if (bad()) {  // error was set
		fclose(fsrc);  // don't check these - there's nothing we can do about them
		fclose(fdest);
		unlink(to.c_str());

		return false;
	}

	if (fclose(fsrc) == -1) {
		fclose(fdest);
		unlink(to.c_str());

		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Error while closing source file: /errno/."), errno, path_, to);
		return false;
	}

	if (fclose(fdest) == -1) {  // the OS may delay writing until this point (or even further).
		unlink(to.c_str());

		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Error while closing source file: /errno/."), errno, path_, to);
		return false;
	}

	// copy permissions. don't check for errors here - they are harmless.
	if (stat_result == 0)
		chmod(to.c_str(), st.st_mode & 0777);  // don't transfer unneeded stuff.

	return true;
}






}  // ns hz



#endif
