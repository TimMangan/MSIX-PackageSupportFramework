#pragma once
// NOT TO BE INCLUDED DIRECTLY BY ANY SOURCE FILE

/***********************************************************************************************************
Internal documentation for RegLegacyFixups

There are some general principles used within this fixup.

1.  We try to avoid reentrancy by using a thread local reentrancy guard. This is a big hammer, but registry
   operations can be very deep and complex, and it is hard to predict all the ways that reentrancy can occur.
   The reentrancy guard will prevent any fixup code from running if we are already inside a fixup on the same thread.
   This means that some registry operations may not get fixed up if they are called from within another fixup,
   but this is preferable to infinite recursion or stack overflows.
2.  We log at various levels of detail, controlled by the g_JsonDebugLevel variable. This allows us to see what is going on.  The levels used here are:
   - LogLevel_Exception: Used for logging exceptions that occur within the fixup.
   - LogLevel_Launching: NOT USED IN THIS FIXUP.
   - LogLevel_DebugBasic: Basic information about the function call and its parameters.
   - LogLevel_DebugIntermediate: More detailed information, including results of operations.
   - LogLevel_DebugMaximum: Very detailed information, including internal state and decisions made.
3. Ultimately, the registry in the kernel will use wide characters.  Many, but not all of the functions that we intercept have both ANSI and wide character versions.  
   In this module, we will intercept both the wide and ansi functions, but implement a wide handler where all the complicated logic lives.  
   The wide intercept will just log the input/output and call the helper.
   The ansi intercept will log the input/output but widen string before calling the helper.
4. The helper functions will first consider DeletionMarkers and JavaMarkers before considering any redirection.   
   Whenever possible, the helper function will use the impl:: functions to avoid reentrancy into the fixup layer, however when not possible the guard will prevent reentrancy.
5. When redirection is involved, we will open existing items where they live, keeping in mind that we have have received a non-redirected or redirected key as input and
   we may need to consider the other side as well. 


   *******************************************************************************************************************************************************************/