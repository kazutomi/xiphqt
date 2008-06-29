<?
print_r($_FILES);
?>
<hr>
<?
print_r($_POST);

$fpath = "/var/www/uploads/";
$file_name = $fpath . basename($_FILES{'myfile'}{'name'});
echo $file_name;

// move (actually just rename) the temporary file to the real name
move_uploaded_file ( $_FILES{'myfile'}{'tmp_name'}, $file_name );

// convert the uploaded file back to binary

// javascript "escape" does not encode the plus sign "+", but "urldecode"
//	in PHP make a space " ". So replace any "+" in the file with %2B first

$filename = $file_name;
$handle = fopen($filename, "r");
$contents = fread($handle, filesize($filename));
fclose($handle);

$contents = preg_replace("/\+/", "%2B", $contents);

$handle = fopen($filename, "w");
fwrite($handle, urldecode($contents));
fclose($handle);

?>


