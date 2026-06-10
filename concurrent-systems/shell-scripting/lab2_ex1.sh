echo "START"

# יצירת התיקייה והכניסה אליה
mkdir $1
cd $1

# יצירת קובץ עם 5 שורות ראשונות + מפריד + 5 שורות אחרונות של man ls
man ls | col -b | (head -n 5; echo "....."; tail -n 5) > $USER.txt

# הורדת הרשאות כתיבה
chmod -w $USER.txt

# יצירת קובץ home.txt עם שם תיקיית HOME
echo "$HOME" > home.txt

echo "END"