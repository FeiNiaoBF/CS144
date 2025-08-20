#include "byte_stream_test_harness.hh"
#include "random.hh"

#include <algorithm>
#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    const size_t CAPACITY = 42;

    // Verify that once the Writer is closed, no more data can be pushed.
    {
      ByteStreamTestHarness test { "push after close", CAPACITY };

      test.execute( Push { "144" } );
      test.execute( BytesPushed { 3 } );
      test.execute( BufferEmpty { false } );
      test.execute( IsClosed { false } );
      test.execute( IsFinished { false } );

      // Close the writer.
      test.execute( Close {} );
      test.execute( IsClosed { true } );
      test.execute( IsFinished { false } );

      // Attempt to push after closed. Should NOT push anything.
      test.execute( Push { "an_arbitrary_string" } );

      // The total number of pushed bytes should remain 3.
      test.execute( BytesPushed { 3 } );
      test.execute( BufferEmpty { false } );
    }

  } catch ( const exception &e ) {
    cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
