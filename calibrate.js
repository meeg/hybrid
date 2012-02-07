// gui.setValue         ( name, value );            // Set variable value
// gui.sendCommand      ( command, arg );           // Send command
// gui.setDefaults      ();                         // Set defaults
// gui.loadConfig       ( file );                   // Load configuration from file
// gui.saveConfig       ( file );                   // Save configuration to file
// gui.openFile         ( file );                   // Open data file
// gui.closeFile        ();                         // Close data file
// gui.setRunParameters ( rate, count );            // Set run parameters
// gui.setRunWait       ( uint time );              // Set time before run start (in mS, min=1mS)
// gui.iter             ( );                        // Returns iteration count

// Run baseline and calibrations
//gui.setDefaults();
gui.setRunParameters("100Hz",4000);
gui.setRunWait(1000);

var pre = "13";
print("Starting Calibration. Base=" + pre);

// Before run callback
// Return new runState
// Return empty string to leave state unchanged and end script
gui.run = function () {
   gui.closeFile();

   // Done after 9 iterations
   if ( gui.iter() >= 9 ) {
      print("Done.");
      return(""); // Stop run
   }

   if ( gui.iter() == 8 ) {
      gui.setValue("cntrlFpga:hybrid:apv25:CalibInhibit","True");
      gui.setValue("cntrlFpga:hybrid:apv25:CalGroup","0");
      gui.openFile("data/" + pre + "_baseline.bin");
      print("Running baseline");
   }
   else {
      gui.setValue("cntrlFpga:hybrid:apv25:CalibInhibit","False");
      gui.setValue("cntrlFpga:hybrid:apv25:CalGroup",gui.iter());
      gui.openFile("data/" + pre + "_cal_" + gui.iter() + ".bin");
      print("Running cal. Group: " + gui.iter());
   }

   return ("Running"); // Start run
}

