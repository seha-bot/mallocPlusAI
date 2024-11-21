#ifndef MALLOC_PLUS_AI
#define MALLOC_PLUS_AI

#define MALLOC_PLUS_AI_API_KEY "YOUR_API_KEY"

///
/// Though malloc is a very useful function in c, it is not without its
/// problems. The biggest is that it can be confusing for some to decide how
/// much memory to allocate, needing complicated statements with sizeof.
/// To solve this I propose a new alternative to malloc that utilizes the power
/// of modern developments in AI, mallocPlusAI.
///
/// The usage is simple:
/// 	int *x = (int *)mallocPlusAI("Enough memory to store up to 5 integers");
///
/// If this function miraculously returns a valid pointer, using it would not be
/// the greatest idea! Don't forget to free and good luck.
///
void *mallocPlusAI(const char *prompt);

#endif
