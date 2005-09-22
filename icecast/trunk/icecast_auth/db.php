<?php
class DB {
	function DB($ihost, $iuser, $ipass, $idb) {
		$this->host = $ihost;
		$this->db = $idb;
		$this->user = $iuser;
		$this->pass = $ipass;
		$this->link = mysql_connect($this->host, $this->user, $this->pass);
		mysql_select_db($this->db);
		register_shutdown_function(array( &$this, "closedb" ));
	}
	function closedb() {
		mysql_close($this->link);
	}
}
?>