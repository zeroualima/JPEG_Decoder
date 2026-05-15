import subprocess
import pytest
import re
import logging
import signal
from pathlib import Path
from itertools import product
from skimage.metrics import structural_similarity as ssim
import numpy as np
from PIL import Image


logger = logging.getLogger(__name__)
root_dir=Path(__file__).parent.parent.resolve()
images_dir = root_dir/"images"
etu_dir = images_dir/"etu"
out_dir = root_dir/"out"
test_dir = root_dir/"tests"
exe=str(root_dir/"ppm2jpeg")
invader = etu_dir/"invader.pgm"
zz = etu_dir/"zig-zag.ppm"
invader_jpg =  out_dir/"invader.jpg"
motif = "../out/"
bloated_invader = f"{invader_jpg.parent}/" + motif * ((4095 - len(str(invader_jpg)))//len(motif)) + invader_jpg.name


### Pour tester le progressif
bisou = etu_dir/"bisou.pgm" 
zigzag = etu_dir/"zig-zag.ppm"
horizontal = etu_dir/"horizontal.ppm"
bisou_jpg =  out_dir/"bisou.jpg"
zigzag_jpg = out_dir/"zig-zag.jpg"
horizontal_jpg = out_dir/"horizontal.jpg"
mode_prog = "p"


def pytest_error(message, command="", stdout="", stderr=""):
    """Formatage propre des erreurs pour l'étudiant."""
    command_str = " ".join(command)
    full_msg = f"\n[COMMANDE] : {command_str}"
    full_msg += f"\n[CONSTAT]  : {message}"
    if stdout: full_msg += f"\n[STDOUT] : {stdout}"
    if stderr: full_msg += f"\n[STDERR] : {stderr}"
    pytest.fail(full_msg, pytrace=False)
    
def test_c_unity(subtests, request):
    # Liste des binaires de tests générés
    for exe_path in test_dir.glob("test_*.bin"):
        res = subprocess.run( [exe_path], capture_output=True, text=True)
        # récupération du résumé du test Unity pour le hook
        match = re.search(r"(\d+ Tests \d+ Failures \d+ Ignored)", res.stdout)
        summary = match.group(1) if match else "No Unity summary"
        with subtests.test(msg=f"{exe_path.stem.removeprefix("test_")} : {summary}"):
            # Synthèse des erreurs
            if res.returncode != 0:
                # Format Unity : <fichier>:<ligne>:<nom_test>:FAIL:<message>
                failures = re.findall(r".+:\d+:.+:FAIL:.+", res.stdout)
                error_msg = "\n".join(failures)
                pytest.fail(f"{error_msg}", pytrace=False)


def param_images(path, se=None):
    path_se = f"{path.stem}-{se}.jpg" if se is not None else f"{path.stem}.jpg"
    return [exe, f"--outfile={out_dir}/{path_se}", str(path)]

def est_valide(sous_ech):
    h1, v1, h2, v2, h3, v3 = sous_ech 
    return (h1*v1 + h2*v2 + h3*v3 <= 10 and
            h1 % h2 == 0 == h1 % h3 and
            v1 % v2 == 0 == v1 % v3)

def to_str(se):
    return f"{se[0]}x{se[1]},{se[2]}x{se[3]},{se[4]}x{se[5]}"

def get_scenario_dict_base(f, id_fails=[], no_quality=[]):
    return {
        "id": f.stem,
        "cmd": param_images(f),
        "should_succeed": f.stem not in id_fails,
        "ref": str(f),
        "out": f"{out_dir}/{f.stem}.jpg" if f.stem not in no_quality else None
    }

def get_scenario_dict(f, se):
    return {
        "id": f"{f.stem} - {to_str(se)}",
        "cmd": param_images(f,to_str(se))+[f"--sample={to_str(se)}"],
        "ref": str(f),
        "out": f"{out_dir}/{f.stem}-{to_str(se)}.jpg",
        "se" : to_str(se),
        "should_succeed": est_valide(se)
    }

def run_and_check(scenario):
    command = scenario["cmd"]
    # On logue la commande au niveau INFO (pytest --log-cli-level=INFO)
    logger.info(f"Commande : {' '.join(command)}")
    success_expected = scenario.get("should_succeed", True)
    pattern = scenario.get("expect_pattern", "")
    timeout = scenario.get("timeout", 10.0)
    # --- Exécution sous limite de temps ---
    try:
        res = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        stdout_partial = e.stdout.decode() if e.stdout else ""
        stderr_partial = e.stderr.decode() if e.stderr else ""
        pytest_error("TIMEOUT : Le programme a mis trop de temps à répondre (boucle infinie ?)", command, stdout_partial, stderr_partial)
    out, err = res.stdout.strip(), res.stderr.strip()
    # --- VÉRIFICATION 1 : CRASH BRUTAL (segfault, abort) ---
    if res.returncode < 0:
        if res.returncode == -signal.SIGABRT and re.search("Assertion", err, re.IGNORECASE) and not success_expected:
            pytest_error(f"ASSERTION au lieu d'un code d'erreur !", command, out, err)
        else:   
            pytest_error(f"CRASH (Signal {abs(res.returncode)})", command, out, err)
    # --- VÉRIFICATION 2 : CODE DE RETOUR ---
    if success_expected and res.returncode != 0:
        pytest_error(f"CODE DE RETOUR : erreur ({res.returncode}) au lieu d'un succès !", command, out, err)
    if not success_expected and res.returncode == 0:
        pytest_error("CODE DE RETOUR : succès au lieu d'une erreur", command, out, err)
    # --- VÉRIFICATION 3 : SORTIE FLUX (stdout vs stderr) ---
    if success_expected:
        if err:
            pytest_error("ERREUR DE FLUX : pas de messages d'erreurs en cas de succès !", command, out, err)
        if not re.search(pattern, out, re.IGNORECASE):
            pytest_error(f"ATTENDU dans stdout : {pattern}", command, out, err)
    else:
        if not err:
            pytest_error("ERREUR DE FLUX : absence de message d'erreur dans stderr.", command, out, err)
        if not re.search(pattern, err, re.IGNORECASE):
            pytest_error(f"ATTENDU dans stderr : {pattern}", command, out, err)
    # --- VÉRIFICATION 4 : FICHIER GENERE ---
    filename = scenario.get("out")
    if success_expected and filename is not None:
        if not Path(filename).is_file():
            pytest_error(f"Fichier {filename} manquant !", command, out, err)
        cmd = ["identify" , "-verbose", filename]
        res = subprocess.run(cmd, capture_output=True, text=True)
        format_identify = "Format: JPEG (Joint Photographic Experts Group JFIF format)"
        if format_identify not in res.stdout:
            pytest_error(f"{filename} n'est pas un JPEG/JFIF !", command, out, err)
        extension = Path(scenario["ref"]).suffix
        if extension == ".pgm":
            se = "1x1"
        else:
            se = scenario.get("se","1x1,1x1,1x1")
        if re.search(r"jpeg:sampling-factor:\s*([\d,x]+)", res.stdout).group(1) != se:
            pytest_error(f"{filename} ne produit pas le sous-échantillonnage {se} !", command, out, err)


def compare_SSIM(ppm_path, res_path, convert_path, se):
    ppm = np.array(Image.open(ppm_path))
    res = np.array(Image.open(res_path))
    convert = np.array(Image.open(convert_path))
    axis = 2 if len(ppm.shape) == 3 else None
    score = ssim(ppm, res, channel_axis=axis, data_range=255)
    score_convert = ssim(ppm, convert, channel_axis=axis, data_range=255)
    test = score < score_convert - 0.05
    msg = f"[SSIM] Qualité d'image insuffisante : {score} au lieu de {score_convert} pour convert !"
    return score, test, msg

def compare_PAE(ppm_path, res_path, convert_path, se):
    command = ["compare", "-metric", "PAE", ppm_path, res_path, "null:"]
    res = subprocess.run(command, capture_output=True, text=True)
    match = re.search(r"\((.*?)\)", res.stderr) # BEURK compare écrit sur stderr
    score = float(match.group(1))
    test = score > 0.5 # Pour info, convert passe à 0.23 sans sous-ech
    msg  = f"[PAE] {score} : Y a du pixel qui saute !"
    return score, test, msg

def compare_AE(ppm_path, res_path, convert_path, se):
    if se:
        return float("nan"), False, f"Test non pertinent !"
    command = ["compare", "-metric", "AE", "-fuzz", "10%", ppm_path, res_path, "null:"]
    res = subprocess.run(command, capture_output=True, text=True)
    nb_pixels_erreur = int(res.stderr)
    dim_cmd = ["identify", "-format", "%w %h", ppm_path]
    dim_res = subprocess.run(dim_cmd, capture_output=True, text=True)
    w, h = map(int, dim_res.stdout.split())
    total_pixels = w * h
    pourcentage = (nb_pixels_erreur / total_pixels) * 100
    test = pourcentage > 2. # Pour info, convert passe à 0.1 
    msg  = f"[AE] {pourcentage}% de pixels à plus de 10% de l'original !"
    return pourcentage, test, msg

METRICS_MAP = {
    "SSIM": compare_SSIM,
    "PAE": compare_PAE,
    "AE": compare_AE
}

# Liste des scénarii d'intégration
SCENARII = {}
SCENARII["cli"] = [
    {
        "id": "Sans argument",
        "cmd": [exe],
        "should_succeed": False
    },
    {
        "id": "A l'aide",
        "cmd": [exe, "--help"],
        "expect_pattern": "Usage"
    },
    {
        "id": "Invader sans options, chemin relatif",
        "cmd": [exe, str(invader.relative_to(root_dir))],
        "ref": str(invader),
        "out": str(invader_jpg)
    },
    {
        "id": "Invader sans options, chemin absolu",
        "cmd": [exe, str(invader)],
        "ref": str(invader),
        "out": str(invader_jpg)
    },
    {
        "id": "Invader avec --outfile, chemin relatif",
        "cmd": [exe, f"--outfile={invader_jpg.relative_to(root_dir)}", str(invader)],
        "ref": str(invader),
        "out": str(invader_jpg)
    },
    {
        "id": "Invader avec --outfile, chemin absolu",
        "cmd": param_images(invader),
        "ref": str(invader),
        "out": str(invader_jpg)
     },
    {
        "id": "Invader avec --outfile, bloated",
        "cmd": [exe, f"--outfile={bloated_invader}", str(invader)],
        "ref": str(invader),
        "out": str(invader_jpg)
    },
    # TESTS DU MODE PROGRESSIF
    {
        "id": "Invader avec --mode",
        "cmd": [exe, f"--mode={mode_prog}", str(invader)],
        "ref": str(invader),
        "out": str(invader_jpg)
    },
    {
        "id": "Bisou avec --mode",
        "cmd": [exe, f"--mode={mode_prog}", str(bisou)],
        "ref": str(bisou),
        "out": str(bisou_jpg)
    },
    {
        "id": "Zig-Zag avec --mode",
        "cmd": [exe, f"--mode={mode_prog}", str(zigzag)],
        "ref": str(zigzag),
        "out": str(zigzag_jpg)
    },
    {
        "id": "Horizontal avec --mode",
        "cmd": [exe, f"--mode={mode_prog}", str(horizontal)],
        "ref": str(horizontal),
        "out": str(horizontal_jpg)
    }
]

SCENARII["gris"] = [
    get_scenario_dict_base(f)
    for f in sorted(etu_dir.rglob("*.pgm"))
]

SCENARII["couleur"] = [
    get_scenario_dict_base(f)
    for f in sorted(etu_dir.rglob("*.ppm"))
]

# Sous-ech de base sur tout sauf biiig et zz
SCENARII["se-base"] = [
    get_scenario_dict(f,se)
    for f in sorted(etu_dir.rglob("*.ppm")) if str(f.stem) not in ["biiiiiig","zig-zag"]
    for se in [(2,2,1,2,1,2), (2,2,2,1,2,1), (1,2,1,1,1,1), (2,1,1,1,1,1), (2,2,1,1,1,1)]
]
# Sous-ech le grand jeu sur zz TODO echange zz-vertical pour plus de violence ?
scenarii_subsampling = [
    get_scenario_dict(f,se)
    for f in [zz]
    for se in product(range(1, 5), repeat=6)
]

SCENARII["se-full"] = [
s for s in scenarii_subsampling if s["should_succeed"]]

SCENARII["se-illegal"] = [
s for s in scenarii_subsampling if not s["should_succeed"]]
    
@pytest.fixture
def quality_check(request, add_metric):
    """Compare l'image générée selon plusieurs métriques avec le ppm/pgm source
    et/ou une image obtenue avec convert"""
    # Fixture factory pour garder l'accès aux autres fixtures
    def _check(category,scenario):
        if category not in ["gris","couleur","se-base","se-full"]: return
        ppm = scenario.get("ref")
        jpg = scenario.get("out")
        se = scenario.get("se")
        id=scenario["id"]
        out_path=Path(jpg)
        convert_path=out_path.parent / f"ref_{out_path.name}"
        convert=str(convert_path)
        if not convert_path.is_file():
            if se:
                cmd = ["convert", ppm, "-sampling-factor", se, convert]
            else:
                cmd = ["convert", ppm, convert]
            result = subprocess.run(cmd, capture_output=True)
            if result.returncode != 0:
                pytest.fail(f"Impossible de générer la référence : {result.stderr.decode()}")
        failed = False
        error_msg = ""
        for metric in METRICS_MAP.keys():
            score, test, msg = METRICS_MAP[metric](ppm, jpg, convert, se)
            add_metric(category, id, metric, score, test)
            failed = failed or test
            if test:
                error_msg += f"{id} : {msg}\n"
        if failed:
            pytest.fail(error_msg, pytrace=False)
        return 
    return _check


@pytest.mark.parametrize("category", SCENARII.keys())
def test_integration(category, quality_check, subtests):
    if category == "se-illegal":
        for scenario in SCENARII[category]:
            run_and_check(scenario)
    else:
        for scenario in SCENARII[category]:
            with subtests.test(msg=scenario["id"]):
                run_and_check(scenario)
                quality_check(category,scenario)



@pytest.mark.parametrize("category", ["couleur"])
def test_performance(request, category, add_metric, subtests):
    for scenario in SCENARII[category]:
        jpg = scenario["out"]
        cg_file = f"callgrind.{Path(jpg).stem}.out"
        with subtests.test(msg=scenario["id"]):
            cmd = ["valgrind", "--tool=callgrind", f"--callgrind-out-file={cg_file}"]
            cmd += scenario["cmd"]
            # Callgrind pour récupérer le nombre d'instructions 
            res = subprocess.run(cmd, capture_output=True, text=True)
            instructions = int(re.search(r"Collected\s*:\s*(\d+)", res.stderr).group(1))
            # Identify pour récupérer les dimensions de l'image
            cmd2 = ["identify" , jpg]
            res2 = subprocess.run(cmd2, capture_output=True, text=True)
            match = re.search(r"JPEG.*?(\d+)x(\d+)", res2.stdout)
            score = instructions / int(match.group(1)) / int(match.group(2))
            failed = score > 3000 # Implem naive à 6500, cachée à 1500, Loeffler à 750
            add_metric(category, scenario["id"], "Perf(instructions/pixel)", score, failed)
            if failed:
                pytest.fail("Implem pas encore efficace !", pytrace=False)

LIMITS = {
    ".text":  {
        "fail": lambda val, metrics: val > 50,
        "msg" : "Au delà de 50ko, faut sérieusement penser à factoriser !"
     },
    ".rodata":{
        "fail": lambda val, metrics: val > 50,
        "msg" : "Au delà de 50ko, ça sent la tabulation massive !"
     },
    ".bss":   {
        "fail": lambda val, metrics: val > 2048,
        "msg" : "2Mo de tableaux statiques... Malaise !"
     },
    "heap":   {
        "fail": lambda val, metrics: val > 5000,
        "msg" : "Pile excessive. Il faut réfléchir à charger l'image différemment."
     },
    "stack":  {
        "fail": lambda val, metrics: val > 32,
        "msg" : "Stackoverflow, ca vous parle ?"
     },
    "extra":  {
        "fail": lambda val, metrics: metrics["extra"]*100 > metrics["heap"]*25,
        "msg" : "Trop de petits mallocs (max 25%)"
     },
    "RSS":  {
        "fail": lambda val, metrics: val > 5120,
        "msg" : "Empreinte mémoire physique trop élevée"
     }
}

def str2Ko(s):
    return int(s)//1024

def get_massif_peak():
    output_file = "massif.out"
    subprocess.run(["valgrind", "--tool=massif", "--stacks=yes", f"--massif-out-file={output_file}", exe, str(etu_dir/"biiiiiig.ppm")], check=True)
    peak_total = -1
    metrics = (0, 0, 0) # (heap, extra, stack) 
    with open(output_file, 'r') as f:
        for line in f:
            if line.startswith("mem_heap_B="):
                heap  = str2Ko(line.split("=")[1])
                extra = str2Ko(next(f).split("=")[1])
                stack = str2Ko(next(f).split("=")[1])
                total = heap + extra + stack
                if total > peak_total:
                    peak_total = total
                    metrics = (heap, extra, stack)
    return metrics

def get_size_sections():
    res = subprocess.run(["size", "-A", exe], capture_output=True, text=True)
    sections = {}
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2:
            name = parts[0]
            size_bytes = parts[1]
            if size_bytes.isdigit():
                sections[name] = str2Ko(size_bytes)
    return sections

def get_rss():
    res = subprocess.run(["/usr/bin/time", "-v", exe, str(etu_dir/"biiiiiig.ppm")], capture_output=True, text=True)
    return int(re.search(r"Maximum resident set size.*:\s*([\d]+)", res.stderr).group(1))


def test_memory(request, add_metric):
    """Vérifie que chaque section mémoire ne dépasse pas son quota."""
    failed = False
    error_msg = ""
    mem_metrics = get_size_sections()
    massif_metrics = ("heap", "extra", "stack")
    mem_metrics.update(zip(massif_metrics, get_massif_peak()))
    mem_metrics["RSS"] = get_rss()
    for m, constraint  in LIMITS.items(): 
        score = mem_metrics[m]
        test = constraint["fail"](score,mem_metrics)
        add_metric("Empreinte mémoire", m, "Ko", score, test)
        failed = failed or test
        if test:
            error_msg += f"{m} : {score} Ko ! {constraint['msg']}\n"
    if failed:
        pytest.fail(error_msg, pytrace=False)
