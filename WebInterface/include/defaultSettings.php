<?php
session_start();

// get defaultSettings over control.js @ parseResponse - not typical to solve this here but who cares :)
if (isset($_GET) AND !empty($_GET)) {
	foreach ($_GET AS $SETTING => $VALUE) {
		$_SESSION["$SETTING"] = $VALUE;

		if (isset($DEBUG) AND $DEBUG == 1) {
			$filename = "/tmp/settings.conf";
			if (is_writable($filename)) {
				$handle = fopen($filename, "a");
				$write = fwrite($handle, "$SETTING = $VALUE\n");
				fclose($handle);
			}
		}

	}
}


?>