<?php

class genfile
{
    protected static $path = "/tmp";
    
    /**
     * Writes a genfile on the disk, atomically (write + mv).
     * 
     * @param string $name The name of the genfile
     * @param mixed $data The data to write
     * @param bool $no_serializing
     * @return bool
     */
    public static function write($name, $data, $no_serializing = false)
    {
        // Open the temporary file
	    $fp = fopen(self::$path . '/' . $name . '.tmp', 'w');
	    if (!$fp)
	    {
		    return false;
	    }
	    
	    // Serialize the data, if needed
	    if (!$no_serializing)
	    {
    	    $data = serialize($data);
	    }
	    
	    // Write the data
	    $l = fwrite($fp, $data);
	    fclose($fp);
	    if ($l != strlen($data))
	    {
		    @unlink(self::$path . '/' . $name . '.tmp');
		
		    return false;
	    }

	    // Move the file to its definitive location (for atomicity)
	    if (!rename(self::$path . '/' . $name . '.tmp',
	                self::$path . '/' . $name))
	    {
		    return false;
	    }
	    
	    return true;
    }
    
    /**
     * Retrives a genfile.
     * 
     * @param string $name The name of the genfile
     * @param bool $no_unserialize
     * @return mixed
     */
    public static function get($name, $no_unserialize = false)
    {
        $data = @file_get_contents(self::$path . '/' . $name);
        
        if ($data !== false)
        {
            if (!$no_unserialize)
            {
                $data_u = @unserialize($data);
                
                if ($data_u === false && $data !== serialize(false))
                {
                    return null;
                }
                else
                {
                    $data = $data_u;
                    unset($data_u);
                }
            }
            
            return $data;
        }
        else
        {
            return null;
        }
    }
    
    /**
     * Make a full genfile name by prepend the environment to it.
     * 
     * @param string $base_name
     * @return string
     */
    public static function makeGenfileName($base_name)
    {
        if (!defined('ENVIRONMENT'))
        {
            throw new EnvironmentUndefinedException();
        }
        
        return sprintf("%s_%s.genfile", ENVIRONMENT, $base_name);
    }
}

?>
