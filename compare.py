import csv

def read_bom(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f, delimiter="\t")
        return list(reader)

old_bom = read_bom("old.tsv")
new_bom = read_bom("new.tsv")

old_dict = {row["No."]: row for row in old_bom}
new_dict = {row["No."]: row for row in new_bom}

for no, new_row in new_dict.items():
    if no in old_dict:
        old_row = old_dict[no]
        for key in new_row:
            if key not in ["LCSC Stock", "LCSC Price", "JLCPCB Stock", "JLCPCB Price"]:
                if new_row[key] != old_row[key]:
                    print(f"Row {no} ({new_row['Designator']}), column {key}: '{old_row[key]}' -> '{new_row[key]}'")
    else:
        print(f"Added row {no}")

for no in old_dict:
    if no not in new_dict:
        print(f"Removed row {no}")
