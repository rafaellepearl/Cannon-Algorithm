#!/bin/bash

# Array con i valori di N da testare
VALORI_N=(4 200 500 800 1000 2000 4000 5000 8000 10000 15000 20000)

# Nome del file di log (cancella il vecchio file se esiste per partire da zero)
LOG_FILE="test.log"
rm -f "$LOG_FILE"

echo "=== Avvio dei Benchmark per la Moltiplicazione di Matrici ==="
echo "I risultati verranno salvati in: $LOG_FILE"
echo "------------------------------------------------------------"

# Loop attraverso tutti i valori di N
for N in "${VALORI_N[@]}"; do
    echo "Esecuzione test per N = $N..."
    
    # Scrittura del separatore e del valore corrente di N nel file di log
    echo "==================================================" >> "$LOG_FILE"
    echo "N = $N" >> "$LOG_FILE"
    echo "==================================================" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"

    # 1. Generazione delle matrici A e B
    echo "[1/4] Generazione matrici..." >> "$LOG_FILE"
    ./build/generator "$N" >> "$LOG_FILE" 2>&1
    echo "--------------------------------------------------" >> "$LOG_FILE"

    # 2. Esecuzione algoritmo sequenziale
    echo "[2/4] Esecuzione sequenziale..." >> "$LOG_FILE"
    ./build/mul_sequential >> "$LOG_FILE" 2>&1
    echo "--------------------------------------------------" >> "$LOG_FILE"

    # 3. Esecuzione algoritmo parallelo (con 4 processi)
    echo "[3/4] Esecuzione parallela (MPI -n 4)..." >> "$LOG_FILE"
    mpirun -n 4 ./build/mul_parallel >> "$LOG_FILE" 2>&1
    echo "--------------------------------------------------" >> "$LOG_FILE"

    # 4. Verifica della correttezza dei risultati (script Python)
    echo "[4/4] Verifica correttezza risultati..." >> "$LOG_FILE" 
    python3 mul_check.py >> "$LOG_FILE" 2>&1
    
    # Aggiunge degli spazi vuoti alla fine di ogni blocco N per distanziare il successivo
    echo "" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
done

echo "------------------------------------------------------------"
echo "✅ Tutti i test sono terminati! Apri '$LOG_FILE' per vedere i log."
