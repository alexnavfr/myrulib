<?php

require_once 'genres.php';

function strtolowerEx($str){
 $result = $str;
 global $strtolowerEx_pairs;
 if(!isset($strtolowerEx_pairs)){
  $from = 'А Б В Г Д Е Ё Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я A B C D E F G H I J K L M N O P Q R S T U V W X Y Z';
  $to =   'а б в г д е ё ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я a b c d e f g h i j k l m n o p q r s t u v w x y z';
  $from = explode(' ', trim($from));
  $to = explode(' ', trim($to));
  $cfrom = count($from);
  $cto = count($to);
  $count = $cfrom > $cto ? $cto : $cfrom;
  for($i = 0; $i < $count; $i++){$strtolowerEx_pairs[$from[$i]] = $to[$i];}
 }
 $result = strtr($str, $strtolowerEx_pairs);
 return $result;
}

function utf8_strlen($s)
{
    return preg_match_all('/./u', $s, $tmp);
}

function strtoupperEx($str){
 $result = $str;
 global $strtoupperEx_pairs;
 if(!isset($strtoupperEx_pairs)){
  $from = 'а б в г д е ё ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я a b c d e f g h i j k l m n o p q r s t u v w x y z Ё';
  $to =   'А Б В Г Д Е Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я A B C D E F G H I J K L M N O P Q R S T U V W X Y Z Е';
  $from = explode(' ', trim($from));
  $to = explode(' ', trim($to));
  $cfrom = count($from);
  $cto = count($to);
  $count = $cfrom > $cto ? $cto : $cfrom;
  for($i = 0; $i < $count; $i++){$strtoupperEx_pairs[$from[$i]] = $to[$i];}
 }
 $result = strtr($str, $strtoupperEx_pairs);
 return $result;
}

function utf8_substr($s, $offset, $len = 'all')
{
    if ($offset<0) $offset = utf8_strlen($s) + $offset;
    if ($len!='all')
    {
        if ($len<0) $len = utf8_strlen($s) - $offset + $len;
        $xlen = utf8_strlen($s) - $offset;
        $len = ($len>$xlen) ? $xlen : $len;
        preg_match('/^.{' . $offset . '}(.{0,'.$len.'})/us', $s, $tmp);
    }
    else
    {
        preg_match('/^.{' . $offset . '}(.*)/us', $s, $tmp);
    }
    return (isset($tmp[1])) ? $tmp[1] : false;
}

function convert_authors($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("DELETE FROM authors");

  $sqlite_db->query("INSERT INTO authors (id, letter, full_name) VALUES(0,'#','(без автора)')");

  $sqltest = "
	SELECT libavtorname.AvtorId, libavtorname.FirstName, libavtorname.LastName, libavtorname.MiddleName, COUNT(libavtor.BookId) as Number
	FROM libavtorname INNER JOIN (
	  SELECT DISTINCT libavtor.AvtorId, libavtor.BookId 
	  FROM libavtor INNER JOIN libbook ON libbook.BookId=libavtor.BookId AND libbook.Deleted<>1 
	) AS libavtor ON libavtorname.AvtorId=libavtor.AvtorId 
	GROUP BY libavtorname.AvtorId, libavtorname.FirstName, libavtorname.LastName, libavtorname.MiddleName
  ";

  $char_list = 'А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ы Э Ю Я A B C D E F G H I J K L M N O P Q R S T U V W X Y Z';

  $query = $mysql_db->query($sqltest);
  while ($row = $query->fetch_array()) {
    $full_name = trim($row['LastName'])." ".trim($row['FirstName'])." ".trim($row['MiddleName']);
    $full_name = str_replace("  ", " ", $full_name);
    $search_name = strtolowerEx($full_name);
    $letter = utf8_substr($full_name,0,1);
    $letter = strtoupperEx($letter,0,1);

    if (strpos($char_list, $letter) === false) { $letter = "#"; };

    echo $row['AvtorId']." - ".$letter." - ".$full_name." - ".$search_name."\n";

    $sql = "INSERT INTO authors (id, number, letter, full_name, search_name, first_name, middle_name, last_name) VALUES(?,?,?,?,?,?,?,?)";
    $insert = $sqlite_db->prepare($sql);
    if($insert === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $err= $insert->execute(array($row['AvtorId'], $row['Number'], $letter, $full_name, $search_name, trim($row['FirstName']), trim($row['MiddleName']), trim($row['LastName'])));
    if($err === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $insert->closeCursor();
  }
}  

function convert_genres($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");
  $sqlite_db->query("DELETE FROM genres");

  $sqltext = "
	SELECT DISTINCT libgenre.BookId, GenreCode 
	FROM libgenre 
		LEFT JOIN libgenrelist ON libgenre.GenreId = libgenrelist.GenreId 
		INNER JOIN libbook ON libbook.BookId=libgenre.BookId AND libbook.Deleted<>1 
	ORDER BY GenreCode
  ";
  $query = $mysql_db->query($sqltext);
  while ($row = $query->fetch_array()) {
    $genre = GenreCode($row['GenreCode']);
    echo $row['GenreCode']." - ".$row['BookId']." - ".$genre."\n";
	if (!empty($genre)) {
		$sql = "INSERT INTO genres(id_book, id_genre) VALUES(?,?)";
		$insert = $sqlite_db->prepare($sql);
		$err = $insert->execute(array($row['BookId'], $genre));
	}
  }
  $sqlite_db->query("commit;");
}

function convert_books($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("DELETE FROM books");
  $sqlite_db->query("DELETE FROM words");

  $sqltest = "
    SELECT 
      libbook.BookId, FileSize, Title, Deleted, FileType, md5, DATE_FORMAT(libbook.Time,'%y%m%d') as Time, Lang, Year,
      CASE WHEN AvtorId IS NULL THEN 0 ELSE AvtorId END AS AvtorId,
      CASE WHEN libfilename.FileName IS NULL THEN 
        CASE WHEN oldfilename.FileName IS NULL THEN CONCAT(libbook.BookId, '.', libbook.FileType) ELSE oldfilename.FileName END
        ELSE libfilename.FileName
      END AS FileName
    FROM libbook 
      LEFT JOIN libavtor ON libbook.BookId = libavtor.BookId
      LEFT JOIN libfilename ON libbook.BookId = libfilename.BookId
      LEFT JOIN oldfilename ON libbook.BookId = oldfilename.BookId
    WHERE libbook.Deleted<>1
  ";

  $query = $mysql_db->query($sqltest);
  while ($row = $query->fetch_array()) {
    echo $row['Time']." - ".$row['BookId']." - ".$row['FileType']." - ".$row['AvtorId']." - ".$row['Title']."\n";

    $genres = "";
    $subsql = "SELECT GenreCode FROM libgenre LEFT JOIN libgenrelist ON libgenre.GenreId = libgenrelist.GenreId WHERE BookId=".$row['BookId'];
    $subquery = $mysql_db->query($subsql);
    while ($subrow = $subquery->fetch_array()) {
      $genres = $genres.GenreCode($subrow['GenreCode']);
    }
    $file_type = trim($row['FileType']);
    $file_type = trim($file_type, ".");
    $file_type = strtolower($file_type);
    $file_type = strtolowerEx($file_type);
    $lang = $row['Lang'];
    $lang = strtolower($lang);
    $lang = strtolowerEx($lang);
    $sql = "INSERT INTO books (id, id_author, title, deleted, file_name, file_size, file_type, genres, created, lang, year, md5sum) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)";
    $insert = $sqlite_db->prepare($sql);
    if($insert === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $err= $insert->execute(array($row['BookId'], $row['AvtorId'], trim($row['Title']), $row['Deleted'], $row['FileName'], $row['FileSize'], $file_type, $genres, $row['Time'], $lang, $row['Year'], $row['md5']));
    if($err === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $insert->closeCursor();
  }
  $sqlite_db->query("commit;");
}  

function convert_dates($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("DELETE FROM dates");

  $sqltest = "
    SELECT DATE_FORMAT(Time,'%y%m%d') as Time, MAX(BookId) as Max, MIN(BookId) as Min, COUNT(BookId) AS Num
    FROM libbook 
	WHERE Deleted<>1
	GROUP BY DATE_FORMAT(libbook.Time,'%y%m%d')
  ";

  $query = $mysql_db->query($sqltest);
  while ($row = $query->fetch_array()) {
	echo $row['Time']." - ".$row['Max']." - ".$row['Min']."\n";
    $sql = "INSERT INTO dates (id, lib_max, lib_min, lib_num) VALUES(?,?,?,?)";
    $insert = $sqlite_db->prepare($sql);
    if($insert === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $err= $insert->execute(array($row['Time'], $row['Max'], $row['Min'], $row['Num']));
    if($err === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $insert->closeCursor();
  }
  $sqlite_db->query("commit;");
}  

function convert_aliases($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("DELETE FROM aliases");

  $sqltest = "SELECT * FROM libavtoraliase ORDER BY AliaseId";

  $query = $mysql_db->query($sqltest);
  while ($row = $query->fetch_array()) {
    echo "alias: ".$row['BadId']." - ".$row['GoodId']."\n";
    $sql = "INSERT INTO aliases(id_author, id_alias) values(?,?)";
    $insert = $sqlite_db->prepare($sql);
    if($insert === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $err= $insert->execute(array($row['BadId'], $row['GoodId']));
    if($err === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $insert->closeCursor();
  }

  $sqlite_db->query("commit;");
}

function convert_seqnames($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("DELETE FROM sequences");

  $sqltest = "
	SELECT libseqname.SeqId, libseqname.SeqName, COUNT(libseq.BookId) as Number
	FROM libseqname INNER JOIN (
	  SELECT DISTINCT libseq.SeqId, libseq.BookId 
	  FROM libseq INNER JOIN libbook ON libbook.BookId=libseq.BookId AND libbook.Deleted<>1 
	) AS libseq ON libseqname.SeqId=libseq.SeqId AND libseq.SeqId<>0
	GROUP BY libseqname.SeqId, libseqname.SeqName
  ";

  $query = $mysql_db->query($sqltest);
  while ($row = $query->fetch_array()) {
    echo $row['SeqId']." - ".$row['SeqName']."\n";
    $sql = "INSERT INTO sequences (id, number, value) VALUES(?,?,?)";
    $insert = $sqlite_db->prepare($sql);
    if($insert === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $err= $insert->execute(array($row['SeqId'], $row['Number'], trim($row['SeqName'])));
    if($err === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $insert->closeCursor();
  }

  $sqlite_db->query("commit;");
}

function convert_sequences($mysql_db, $sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("DELETE FROM bookseq");

  $sqltest = "
    SELECT libseq.BookId, libseq.SeqId, libseq.SeqNumb, libseq.Level
    FROM libseq 
	INNER JOIN libbook ON libseq.BookId = libbook.BookId AND libbook.Deleted<>1
  ";

  $query = $mysql_db->query($sqltest);
  while ($row = $query->fetch_array()) {
    echo $row['SeqId']." - ".$row['BookId']."\n";
    $sql = "INSERT INTO bookseq(id_book, id_seq, number, level) VALUES(?,?,?,?)";
    $insert = $sqlite_db->prepare($sql);
    if($insert === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $err= $insert->execute(array($row['BookId'], $row['SeqId'], $row['SeqNumb'], $row['Level']));
    if($err === false){ $err= $dbh->errorInfo(); die($err[2]); }
    $insert->closeCursor();
  }

  $sqlite_db->query("commit;");
}

function create_tables($sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("
    CREATE TABLE authors(
	id integer primary key,
	letter char(1),
	search_name varchar(255),
	full_name varchar(255),
	first_name varchar(128),
	middle_name varchar(128),
	last_name varchar(128),
	number integer,
	newid integer,
	description text);
  ");

  $sqlite_db->query("
    CREATE TABLE books(
      id integer not null,
      id_author integer not null,
      title varchar(255) not null,
      annotation text,
      genres text,
      id_sequence integer,
      deleted boolean,
      id_archive integer,
      file_name text,
      file_size integer,
      file_type varchar(20),
      md5sum char(32),
      created integer,
      lang char(2),
      year integer,
      description text);
  ");

  $sqlite_db->query("
    CREATE TABLE archives(
      id integer primary key,
      file_name varchar(255),
      file_path varchar(255),
      file_size integer,
      file_count integer,
      min_id_book integer,
      max_id_book integer,
      file_type varchar(20),
      description text);
  ");

  $sqlite_db->query("CREATE TABLE sequences(id integer primary key, number integer, value varchar(255) not null);");

  $sqlite_db->query("CREATE TABLE bookseq(id_book integer, id_seq integer, number integer, level integer, id_author integer);");

  $sqlite_db->query("CREATE TABLE params(id integer primary key, value integer, text text);");
  $sqlite_db->query("DELETE FROM params;");
  $sqlite_db->query("INSERT INTO params(text) VALUES ('Flibusta library');");
  $sqlite_db->query("INSERT INTO params(value) VALUES (1);");
  $sqlite_db->query("INSERT INTO params(text) VALUES ('FLIBUSTA');");
  $sqlite_db->query("INSERT INTO params(id,text) VALUES (11,'flibusta.net');");
  
  $sqlite_db->query("CREATE TABLE aliases(id_author integer not null, id_alias integer not null);");
  
  $sqlite_db->query("CREATE TABLE genres(id_book integer, id_genre CHAR(2));");
  
  $sqlite_db->query("CREATE TABLE dates(id integer primary key, lib_min integer, lib_max integer, lib_num, usr_min integer, usr_max, usr_num integer);");
  
  $sqlite_db->query("commit;");
}

function create_indexes($sqlite_db)
{
  $sqlite_db->query("begin transaction;");

  $sqlite_db->query("CREATE INDEX author_letter ON authors(letter);");
  $sqlite_db->query("CREATE INDEX author_name ON authors(search_name);");

  $sqlite_db->query("CREATE INDEX book_id ON books(id);");
  $sqlite_db->query("CREATE INDEX book_author ON books(id_author);");
  $sqlite_db->query("CREATE INDEX book_archive ON books(id_archive);");
  $sqlite_db->query("CREATE INDEX book_md5sum ON books(md5sum);");
  $sqlite_db->query("CREATE INDEX book_created ON books(created);");

  $sqlite_db->query("CREATE INDEX book_file ON archives(file_name);");

  $sqlite_db->query("CREATE INDEX sequences_name ON sequences(value);");

  $sqlite_db->query("CREATE INDEX bookseq_book ON bookseq(id_book);");
  $sqlite_db->query("CREATE INDEX bookseq_author ON bookseq(id_author);");

  $sqlite_db->query("CREATE INDEX aliases_author ON aliases(id_author);");
  $sqlite_db->query("CREATE INDEX aliases_alias ON aliases(id_alias);");
  
  $sqlite_db->query("CREATE INDEX genres_book ON genres(id_book);");
  $sqlite_db->query("CREATE INDEX genres_genre ON genres(id_genre);");

  $sqlite_db->query("commit;");

  $sqlite_db->query("vacuum;");
}

$mysql_srvr = 'localhost';
$mysql_user = 'root';
$mysql_pass = '';
$mysql_base = 'flibusta';
$sqlitefile = './myrulib.db';

include('settings.php');

$sqlite_db = new PDO('sqlite:'.$sqlitefile);
$mysql_db = new mysqli($mysql_srvr, $mysql_user, $mysql_pass, $mysql_base);
$mysql_db->query("SET NAMES utf8");

create_tables($sqlite_db);
convert_genres($mysql_db, $sqlite_db);
convert_authors($mysql_db, $sqlite_db);
convert_books($mysql_db, $sqlite_db);
convert_dates($mysql_db, $sqlite_db);
convert_seqnames($mysql_db, $sqlite_db);
convert_sequences($mysql_db, $sqlite_db);
create_indexes($sqlite_db);

?>
