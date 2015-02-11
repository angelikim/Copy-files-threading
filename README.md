# Copy-files-threading
Γλώσσα : c++
Εντολή μεταγλώττισης :make

Εντολες εκτέλεσης
  (1) παράδειγμα για τον client : ./client -d (path ) -i (ip) -p (port)
  (2) παράδειγμα για τον server : ./server -p (port) -s (thread number) -q (queue size)
  * τα ορίσματα μπορούν να αλλάξουν σειρά ανα ζεύγη
  
Το makefile συμπεριλαμβάνεται στον φάκελο που παραδόθηκε

ΕΠΙΛΟΓΕΣ ΣΧΕΔΙΑΣΜΟΥ
      O client και ο server ( δύο διαφορετικά εκτελέσιμα ) έχουν σχέση πελάτη -εξυπηρετητή .
      Ο πρώτος θεωρούμε ότι γνωρίζει τα στοιχεία που απαιτούνται για να επικοινωνήσει με τον
      δεύτερο. Αρχικά ο πελάτης στέλνει το μέγεθος του μονοπατιού του φακέλου που θέλει να
      αντιγράψει και μετά το ίδιο το μονοπάτι. Στην περίπτωση που ζητηθεί φάκελος που δεν υπάρχει ,
      ο εξυπηρετητής θα απαντήσει με (-1) , αλλιώς θα σταλεί απάντηση που θα περιέχει τον αριθμό
      σελίδας λειτουργικού .
      
      Για την διαχείριση του μονοπατιού δημιουργείται καινούριο νήμα το οποίο εξυπηρετεί μόνο τον
      συγκεκριμένο πελάτη . Ακόμα για κάθε πελάτη δημιουργείται νέο mutex για την διασφάλιση
      ασφαλούς επικοινωνίας . Το νήμα αυτό με τη σειρά του καλεί την συνάρτηση read_directory() η
      οποία βάζει όλα τα αρχεία του φακέλου αναδρομικά σε μια ουρά αναμονής . Αν υπάρχει χώρος
      στην ουρά αποστολής τότε προσθέτει ένα μονοπάτι αρχείου και ενημερώνει τους εργαζόμενους να
      στείλουν την απάντηση που περιμένει ο πελάτης.Διαφορέτικα περιμένει να ενημερωθεί μέχρι να
      υπάρξει ελεύθερη θέση.
      
      Τα νήματα εργαζόμενοι , αν υπάρχει μονοπάτι στην ουρά αποστολής , βγαίνουν από την
      κατάσταση αναμονής και εξυπηρετούν τους πελάτες παράλληλα , φροντίζοντας κάθε πελάτης να
      επικοινωνεί κάθε φορά μόνο από ένα νήμα εργαζόμενο.

Η ακολουθία αποστολής από τον εξυπηρετητή είναι η εξής:
      *αποστολή μεγέθους μονοπατιού του αρχείου
      *αποστολή μονοπατιού
      *αποστολή μεγέθους αρχείου
      *αποστολή αρχείου ανά μονάδα σελίδας

Η ακολουθία λήψης από τον πελάτη αντίστοιχα είναι η εξής:

      *λήψη μεγέθους μονοπατιού του αρχείου
      *λήψη μονοπατιού
      *λήψη μεγέθους αρχείου
      *λήψη αρχείου ανά μονάδα σελίδας
      (γίνεται διαίρεση με το μέγεθος σελίδας που παρέλαβε αρχικά
      για τον προσδιορισμό του τέλους του αρχείου)ΠΑΡΑΔΟΧΕΣ
      * (-1) θα σταλεί πέρα από την περίπτωση που δεν υπάρχει το αρχείο , όταν έχουν σταλεί όλα τα
      αρχεία , έτσι ώστε ο πελάτης να τερματίσει
      *Για τον τερματισμό λειτουργίας του εξυπηρετητή είτε αποστέλλει ο πελάτης ως μονοπάτι
      “exit” και το ίδιο το νήμα στέλνει σήμα SIGINT στο πρόγραμμα, είτε άμεσα από το τερματικό με
      αποστολή SIGINT.
      * Στον διαχειριστή του σήματος κλείνει το socket , με αποτέλεσμα να μην μπορεί να γίνει λήψη
      αιτήματος από νέο πελάτη και αλλάζει τιμή η μεταβλητή κλάσης flag οδηγώντας τα
      νήματα να τερματίσουν ομαλά.
      *Στην περίπτωση αντιγραφής υπάρχοντος αρχείου , το παλιό διαγράφεται.

ΠΗΓΕΣ ΠΛΗΡΟΦΟΡΙΑΣ

http://www.cplusplus.com/
http://stackoverflow.com/
http://ubuntuforums.org/
