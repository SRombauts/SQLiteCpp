#include <SQLiteCpp/Database.h>
#ifdef SQLITECPP_HAVE_STD_FILESYSTEM
#error filesystem is enabled despite SQLITECPP_DISABLE_STD_FILESYSTEM
#endif
int main()
{
}
