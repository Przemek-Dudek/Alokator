# Alokator
Celem projektu jest przygotowanie managera pamięci do zarządzania stertą własnego programu. W tym celu należy przygotować własne wersje funkcji malloc, calloc, free oraz realloc. Całość należy uzupełnić dodatkowymi funkcjami narzędziowymi, pozwalającymi na monitorowania stanu, spójności oraz defragmentację obszaru sterty.

Przygotowane funkcje muszą realizować następujące funkcjonalności:

Standardowe zadania alokacji/dealokacji zgodne z API rodziny malloc. Należy dokładnie odwzorować zachowanie własnych implementacji z punktu widzenia wywołującego je kodu.
Możliwość resetowania sterty do stanu z chwili uruchomienia programu.
Możliwość samoistnego zwiększania regionu sterty poprzez generowanie żądań dla systemu operacyjnego.
Płotki.
Przestrzeń adresowa pamięci, dla której należy przygotować managera, będzie zawsze zorganizowana jako ciąg stron o długości 4KB.

Funkcje alokujące pamięć muszą uwzględniać płotki bezpośrednio przed i bezpośrednio po bloku przydzielanym użytkownikowi - między nimi nie może być pustych bajtów.

Zadaniem płotków jest ułatwienie wykrywania błędów typu One-off w taki sposób, iż każdy płotek ma określoną i znaną zawartość oraz długość. Jego naruszenie (zamazanie wartości) oznacza, że kod użytkownika niepoprawnie korzysta z przydzielonego mu bloku pamięci i powienien zostać przerwany/poprawiony. Płotek powinien mieć co najmniej 1 bajt, ale zaleca się aby był potęgą 2 i >= 2.
