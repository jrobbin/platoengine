# specify codes
if(ANALYZE)

# Name the test with the directory name
Plato_new_test(TEST_NAME)

# Copy Input files from source to binary dir
set( testFiles "defines.xml;interface.xml;mitchell_tri.xml;alexaApp.xml;platoApp.xml;platoMain.xml;amgx.json;mitchell_tri.gen;mitchell_tri_assoc.txt")
Plato_add_test_files("${testFiles}")

## Plato_add_test_files() only works for files.  *.osh is a directory.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mitchell_tri.osh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


# Set number of processors for PlatoMain comm
set( PLATOMAIN_NP 1 )

# Set number of processors for Performer comms
set( ANALYZE_NP 2 )

# Create list of comm sizes 
set( NUM_PROCS "${PLATOMAIN_NP} ${ANALYZE_NP}" )

# Specify which comm will provide the output for comparison
set( IO_COMM_INDEX 0 )

# Set the name of the interface file
set( INTERFACE_FILE interface.xml )

# Create the test and define the PlatoMain performer
set(APP_FILE platoApp.xml)
set(INPUT_FILE platoMain.xml)
Plato_create_test(PLATO_NEW_TEST ${PLATOMAIN_BINARY} ${PLATOMAIN_NP} ${INTERFACE_FILE} ${APP_FILE} ${INPUT_FILE})

# Define/add a performer
set( LOCAL_COMM_ID 1 )
set(APP_FILE alexaApp.xml)
set(INPUT_FILE --input-config=mitchell_tri.xml)
Plato_add_performer(PLATO_NEW_TEST ${ANALYZE_BINARY} ${ANALYZE_NP} ${LOCAL_COMM_ID} ${INTERFACE_FILE} ${APP_FILE} ${INPUT_FILE})

# Add the test
set(INPUT_MESH mitchell_tri.gen)
set(OUTPUT_MESH platomain.exo)
Plato_add_test(${PLATO_NEW_TEST} ${TEST_NAME} ${NUM_PROCS} ${IO_COMM_INDEX} ${INPUT_MESH} ${OUTPUT_MESH})

endif()
