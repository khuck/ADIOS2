#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.BP3.Mx1.Write
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp3.xml
        Write.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP3.Mx1.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP3.Mx1.Read
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:heatTransfer_read>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp3.xml
        Write.bp Read.bp 1 1
)

add_test(NAME HeatTransfer.BP3.Mx1.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=Read.bp
    -DOUTPUT_FILE=Dump.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP3.Mx1.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    Dump.txt
)

SetupTestPipeline(HeatTransfer.BP3.Mx1 "Write;Read;Dump;Validate" TRUE)

#############################################################################

add_test(NAME HeatTransfer.BP4.Mx1.Write
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
       ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp4.xml
        Write.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP4.Mx1.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.Mx1.Read
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:heatTransfer_read>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp4.xml
        Write.bp Read.bp 1 1
)

add_test(NAME HeatTransfer.BP4.Mx1.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=Read.bp
    -DOUTPUT_FILE=Dump.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP4.Mx1.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    Dump.txt
)

SetupTestPipeline(HeatTransfer.BP4.Mx1 "Write;Read;Dump;Validate" TRUE)
