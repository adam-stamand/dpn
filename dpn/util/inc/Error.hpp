#pragma once

#include <exception>
#include <errno.h>
#include <string.h>

namespace dpn
{
/*  A number random enough not to collide with different errno ranges on      */
/*  different OSes. The assumption is that error_t is at least 32-bit type.   */
#define DPN_HAUSNUMERO 156384712

/*  Native 0MQ error codes.                                                   */
enum
{
    EINVALNAME  = (DPN_HAUSNUMERO + 51),
    ESTLERR
};

const char *errno_to_string (int err)
{
    switch (err) {
        case EINVALNAME:
            return "Invalid name used";
        case ESTLERR:
            return "STL Error";
        default:
            return strerror (err);
    }
}


const char *dpn_strerror (int errnum)
{
    return errno_to_string (errnum);
}


class Error : public std::exception
{
public:
    Error() noexcept : errnum_(dpn_errno()) {}
    explicit Error(int err) noexcept : errnum_(err) {}
    virtual const char *what() const noexcept ZMQ_OVERRIDE
    {
        return dpn_strerror(errnum_);
    }
    int num() const noexcept { return errnum_; }

  private:
    int errnum_;

    int dpn_errno (void)
    {
        return errno;
    }



};

}