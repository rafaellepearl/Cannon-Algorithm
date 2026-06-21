import math
import sys

def verify_matrices(file_seq, file_par, tolerance=1e-5):
    print(f"Confronto in corso tra {file_seq} e {file_par}...")
    
    try:
        with open(file_seq, 'r') as f1, open(file_par, 'r') as f2:
            # Legge tutto il contenuto e lo divide in una lista di stringhe (ignora spazi/accapo)
            data_seq = f1.read().split()
            data_par = f2.read().split()
    except FileNotFoundError as e:
        print(f"❌ Errore: Impossibile trovare uno dei file. Dettagli: {e}")
        return

    # 1. Controllo sulla lunghezza totale
    if len(data_seq) != len(data_par):
        print(f"❌ Errore Dimensionale: {file_seq} ha {len(data_seq)} elementi, {file_par} ne ha {len(data_par)}.")
        return

    # 2. Confronto elemento per elemento con tolleranza
    mismatches = 0
    for i, (val1, val2) in enumerate(zip(data_seq, data_par)):
        try:
            v1, v2 = float(val1), float(val2)
            # math.isclose perdona le piccolissime differenze dei calcoli paralleli
            if not math.isclose(v1, v2, rel_tol=tolerance, abs_tol=tolerance):
                print(f"Discrepanza all'indice {i}: Seq = {v1}, Par = {v2}")
                mismatches += 1
                if mismatches >= 5: # Si ferma dopo 5 errori per non inondare il terminale
                    print("... troppe discrepanze, interrompo la stampa dei log.")
                    break
        except ValueError:
            # Se per qualche motivo (es. intestazioni) ci sono stringhe non numeriche
            if val1 != val2:
                print(f"Discrepanza testuale all'indice {i}: '{val1}' != '{val2}'")
                mismatches += 1

    # 3. Risultato finale
    if mismatches == 0:
        print("✅ SUCCESSO: Le matrici sono matematicamente identiche!")
    else:
        print(f"❌ FALLIMENTO: Trovate {mismatches} discrepanze tra i file.")

if __name__ == "__main__":
    verify_matrices("mul_sequential.txt", "mul_parallel.txt")