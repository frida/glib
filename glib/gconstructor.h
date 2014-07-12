/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a sane way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define _G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define _G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
/* Visual studio 2008 and later has _Pragma */

#define G_HAS_CONSTRUCTORS 1

#if GLIB_SIZEOF_VOID_P == 4
#define _G_PRESERVE_SYMBOL(_symbol) __pragma(comment(linker, "/include:_" G_STRINGIFY (_symbol)))
#else
#define _G_PRESERVE_SYMBOL(_symbol) __pragma(comment(linker, "/include:" G_STRINGIFY (_symbol)))
#endif

#define _G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _wrapper; \
  void _func ## _ (void) { (void) _array ## _func; } \
  _G_PRESERVE_SYMBOL (_func ## _)

#define _G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor; \
  void _func ## _ (void) { (void) _array ## _func; } \
  _G_PRESERVE_SYMBOL (_func ## _)

#elif defined (_MSC_VER)

#define G_HAS_CONSTRUCTORS 1

/* Pre Visual studio 2008 must use #pragma section */
#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define _G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define _G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func ## _wrapper;

#define _G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define _G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define _G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define _G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define _G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define _G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#ifdef GLIB_STATIC_COMPILATION

#define G_DEFINE_CONSTRUCTOR(_func) \
  _G_DEFINE_CONSTRUCTOR (_func ## _register); \
  void _glib_register_constructor (void (*) (void)); \
  static void _func (void); \
  static void _func ## _register (void) { _glib_register_constructor (_func); }
#define G_DEFINE_DESTRUCTOR(_func) \
  _G_DEFINE_CONSTRUCTOR (_func ## _register); \
  void _glib_register_destructor (void (*) (void)); \
  static void _func (void); \
  static void _func ## _register (void) { _glib_register_destructor (_func); }

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  static void _func ## _register (void); \
  _G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS (_func ## _register)
#endif

#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  static void _func ## _register (void); \
  _G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS (_func ## _register)
#endif

#else

#define G_DEFINE_CONSTRUCTOR _G_DEFINE_CONSTRUCTOR
#define G_DEFINE_DESTRUCTOR _G_DEFINE_DESTRUCTOR

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS _G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS
#endif

#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS _G_DEFINE_DESTRUCTOR_PRAGMA_ARGS
#endif

#endif
