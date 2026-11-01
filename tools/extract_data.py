import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile


MAIN = {"s": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
REL_ID = "{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id"
PACKAGE_REL = "{http://schemas.openxmlformats.org/package/2006/relationships}Relationship"


def extract(path, size):
    matrices = []
    names = []

    with zipfile.ZipFile(path) as archive:
        workbook = ET.fromstring(archive.read("xl/workbook.xml"))
        relationships = ET.fromstring(archive.read("xl/_rels/workbook.xml.rels"))
        targets = {item.get("Id"): item.get("Target") for item in relationships.findall(PACKAGE_REL)}
        sheets = workbook.findall("s:sheets/s:sheet", MAIN)[:20]

        if len(sheets) != 20:
            raise ValueError("Expected exactly 20 variant sheets before the summary.")

        for sheet in sheets:
            target = targets[sheet.get(REL_ID)]
            member = target.lstrip("/") if target.startswith("/") else "xl/" + target
            document = ET.fromstring(archive.read(member))
            cells = {}

            for cell in document.findall(".//s:c", MAIN):
                value = cell.find("s:v", MAIN)
                if value is not None:
                    cells[cell.get("r")] = value.text

            matrix = []

            for row in range(size):
                values = []
                for column in range(size):
                    address = f"{chr(66 + column)}{row + 2}"
                    value = int(cells[address])
                    if value < 0 or (size == 10 and value not in (0, 1)):
                        raise ValueError(f"Invalid value at {sheet.get('name')}!{address}")
                    values.append(value)
                matrix.append(values)

            if any(matrix[index][index] != 0 for index in range(size)):
                raise ValueError("Expected a zero diagonal in the supplied adjacency matrices.")

            matrices.append(matrix)
            names.append(sheet.get("name"))

    provenance = {
        "file": path.name,
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "sheets": names,
        "range": "B2:P16" if size == 15 else "B2:K11",
        "convention": "row = source; column = target; zero = no edge",
    }

    return matrices, provenance


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dijkstra", type=Path, required=True)
    parser.add_argument("--warshall", type=Path, required=True)
    arguments = parser.parse_args()

    dijkstra, dijkstra_source = extract(arguments.dijkstra, 15)
    warshall, warshall_source = extract(arguments.warshall, 10)
    output = Path(__file__).resolve().parents[1] / "data"
    output.mkdir(exist_ok=True)

    output.joinpath("graphs.json").write_text(
        json.dumps({"dijkstra": dijkstra, "warshall": warshall}), encoding="utf-8"
    )
    output.joinpath("provenance.json").write_text(
        json.dumps([dijkstra_source, warshall_source], ensure_ascii=False, indent=2), encoding="utf-8"
    )

    print("Extracted 20 Dijkstra variants and 20 Warshall variants.")


if __name__ == "__main__":
    main()
