#!/usr/bin/env python3
"""Gráficos e resumo da bateria de testes (#28), a partir de results/log.csv.

Segue o protocolo da Metodologia do artigo (#26): a execução 1 de cada
configuração é o aquecimento e fica de fora; as três sementes de um mesmo N
são combinadas, e média e desvio padrão amostral saem das medições restantes.

Gera:
  results/figs/tempo.png    tempo x |V|+|E|, um painel por algoritmo, lista e
                            matriz, contra a referência O(V+E)
  results/figs/memoria.png  memória de lista e matriz por tamanho de instância
  results/resumo.csv        média e desvio de tempo e memória por configuração

Uso, a partir de qualquer pasta:
  python3 scripts/graficos.py [results/log.csv]

Requer matplotlib (no Ubuntu: sudo apt install python3-matplotlib).
"""

import csv
import os
import statistics
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
LOG = sys.argv[1] if len(sys.argv) > 1 else os.path.join(RAIZ, "results", "log.csv")
FIGS = os.path.join(RAIZ, "results", "figs")
RESUMO = os.path.join(RAIZ, "results", "resumo.csv")

# Paleta categórica validada (skill dataviz, slots 1 e 2) e tinta de texto.
COR = {"lista": "#2a78d6", "matriz": "#eb6834"}
MARCADOR = {"lista": "o", "matriz": "s"}
LINHA = {"lista": "-", "matriz": "--"}
TINTA = "#0b0b0b"
TINTA_2 = "#52514e"
GRADE = "#e4e3df"
REFERENCIA = "#8a8984"

ALGORITMOS = [
    ("bfs", "BFS"),
    ("dfs", "DFS"),
    ("componentes_fortes", "Componentes fortes"),
    ("componentes_fracos", "Componentes fracos"),
    ("ciclos", "Ciclos"),
    ("bipartido", "Bipartição"),
    ("articulacao", "Pontes e articulação"),
]
ESTRUTURAS = ["lista", "matriz"]


def ler_medicoes(caminho):
    """Agrupa as medições por (algoritmo, estrutura, N), sem o aquecimento."""
    grupos = defaultdict(list)
    with open(caminho, newline="") as f:
        for linha in csv.DictReader(f):
            if int(linha["execucao_num"]) == 1:
                continue
            chave = (linha["algoritmo"], linha["estrutura"], int(linha["N"]))
            grupos[chave].append(
                (int(linha["N"]) + int(linha["M"]), float(linha["tempo_ms"]), float(linha["memoria_kb"]))
            )
    return grupos


def resumir(grupos):
    """Média e desvio padrão amostral de cada configuração."""
    resumo = {}
    for chave, medicoes in grupos.items():
        tamanhos = [m[0] for m in medicoes]
        tempos = [m[1] for m in medicoes]
        memorias = [m[2] for m in medicoes]
        resumo[chave] = {
            "medicoes": len(medicoes),
            "v_mais_e": statistics.mean(tamanhos),
            "tempo_ms": statistics.mean(tempos),
            "tempo_desvio": statistics.stdev(tempos) if len(tempos) > 1 else 0.0,
            "memoria_kb": statistics.mean(memorias),
            "memoria_desvio": statistics.stdev(memorias) if len(memorias) > 1 else 0.0,
        }
    return resumo


def gravar_resumo(resumo):
    with open(RESUMO, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["algoritmo", "estrutura", "N", "medicoes", "v_mais_e_medio", "tempo_ms_medio",
                    "tempo_ms_desvio", "memoria_kb_media", "memoria_kb_desvio"])
        for (algo, est, n), r in sorted(resumo.items()):
            w.writerow([algo, est, n, r["medicoes"], f"{r['v_mais_e']:.1f}", f"{r['tempo_ms']:.4f}",
                        f"{r['tempo_desvio']:.4f}", f"{r['memoria_kb']:.1f}", f"{r['memoria_desvio']:.1f}"])


def estilo_eixo(ax):
    ax.grid(True, which="major", color=GRADE, linewidth=0.6)
    ax.set_axisbelow(True)
    for lado in ("top", "right"):
        ax.spines[lado].set_visible(False)
    for lado in ("left", "bottom"):
        ax.spines[lado].set_color(TINTA_2)
        ax.spines[lado].set_linewidth(0.6)
    ax.tick_params(colors=TINTA_2, labelsize=7, width=0.6)


def referencia(ax, xs, x0, y0, expoente, rotulo):
    """Reta de inclinação `expoente` em log-log, passando por (x0, y0)."""
    ys = [y0 * (x / x0) ** expoente for x in xs]
    ax.plot(xs, ys, color=REFERENCIA, linewidth=0.9, linestyle=":", zorder=1)
    ax.annotate(rotulo, (xs[-1], ys[-1]), xytext=(3, 0), textcoords="offset points",
                fontsize=6.5, color=TINTA_2, va="center")


def figura_tempo(resumo):
    fig, eixos = plt.subplots(2, 4, figsize=(10, 5.2), constrained_layout=True)
    for ax, (algo, titulo) in zip(eixos.flat, ALGORITMOS):
        estilo_eixo(ax)
        for est in ESTRUTURAS:
            pontos = sorted((r["v_mais_e"], r["tempo_ms"], r["tempo_desvio"])
                            for (a, e, _), r in resumo.items() if a == algo and e == est)
            if not pontos:
                continue
            xs, ys, ds = zip(*pontos)
            ax.errorbar(xs, ys, yerr=ds, color=COR[est], marker=MARCADOR[est], markersize=4,
                        linestyle=LINHA[est], linewidth=1.5, capsize=2, elinewidth=0.8,
                        label=est, zorder=3)
            # Só a referência O(V+E), ancorada no menor ponto da lista. A matriz de
            # bits custa V²/64 + E: nas amostras o termo V²/64 é desprezível, e só no
            # grafo completo ele domina. Uma reta V² ancorada no menor ponto
            # exageraria essa diferença; o desvio da matriz acima de O(V+E) já a mostra.
            if est == "lista":
                referencia(ax, xs, xs[0], ys[0], 1, "O(V+E)")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_title(titulo, fontsize=8.5, color=TINTA, loc="left")
        ax.set_xlabel("|V| + |E|", fontsize=7, color=TINTA_2)
        ax.set_ylabel("tempo (ms)", fontsize=7, color=TINTA_2)

    legenda = eixos.flat[-1]
    legenda.axis("off")
    for est in ESTRUTURAS:
        legenda.plot([], [], color=COR[est], marker=MARCADOR[est], linestyle=LINHA[est],
                     linewidth=1.5, markersize=4, label=f"{est} de adjacência")
    legenda.plot([], [], color=REFERENCIA, linestyle=":", linewidth=0.9, label="referência O(V+E)")
    legenda.legend(loc="center", frameon=False, fontsize=7.5, labelcolor=TINTA)
    legenda.text(0.5, 0.08, "média ± desvio padrão\nexecução 1 (aquecimento) descartada",
                 ha="center", fontsize=6.5, color=TINTA_2, transform=legenda.transAxes)

    fig.savefig(os.path.join(FIGS, "tempo.png"), dpi=200, facecolor="white")
    plt.close(fig)


def figura_memoria(resumo):
    """Memória por tamanho, média entre os algoritmos: a estrutura domina a conta,
    e as estruturas auxiliares são as mesmas nas duas representações."""
    por_n = defaultdict(lambda: defaultdict(list))
    for (algo, est, n), r in resumo.items():
        por_n[n][est].append(r["memoria_kb"])
    ns = sorted(por_n)
    rotulos = [f"{n:,}".replace(",", ".") if n <= 1000 else f"completo\n({n:,})".replace(",", ".")
               for n in ns]

    fig, ax = plt.subplots(figsize=(6.4, 3.6), constrained_layout=True)
    estilo_eixo(ax)
    largura = 0.38
    for i, est in enumerate(ESTRUTURAS):
        valores = [statistics.mean(por_n[n][est]) / 1024 for n in ns]
        posicoes = [k + (i - 0.5) * (largura + 0.02) for k in range(len(ns))]
        ax.bar(posicoes, valores, width=largura, color=COR[est], label=f"{est} de adjacência",
               zorder=3, edgecolor="white", linewidth=1)
    for k, n in enumerate(ns):
        lista = statistics.mean(por_n[n]["lista"])
        matriz = statistics.mean(por_n[n]["matriz"])
        ax.annotate(f"{matriz / lista:.1f}×".replace(".", ","), (k + 0.2, matriz / 1024), xytext=(0, 3),
                    textcoords="offset points", ha="center", fontsize=7, color=TINTA)
    ax.set_yscale("log")
    ax.set_xticks(range(len(ns)))
    ax.set_xticklabels(rotulos, fontsize=7)
    ax.set_xlabel("vértices da instância (N)", fontsize=7.5, color=TINTA_2)
    ax.set_ylabel("memória (MB, escala log)", fontsize=7.5, color=TINTA_2)
    ax.set_title("Memória: lista x matriz de adjacência", fontsize=9, color=TINTA, loc="left")
    ax.legend(frameon=False, fontsize=7.5, labelcolor=TINTA, loc="upper left")
    ax.text(1.0, -0.22, "média entre os algoritmos; acima das barras, quantas vezes a matriz usa mais",
            transform=ax.transAxes, ha="right", fontsize=6.5, color=TINTA_2)
    fig.savefig(os.path.join(FIGS, "memoria.png"), dpi=200, facecolor="white")
    plt.close(fig)


def main():
    if not os.path.exists(LOG):
        sys.exit(f"erro: {LOG} não existe; rode antes scripts/bateria.sh")
    os.makedirs(FIGS, exist_ok=True)
    resumo = resumir(ler_medicoes(LOG))
    if not resumo:
        sys.exit(f"erro: {LOG} não tem medições além do aquecimento")
    gravar_resumo(resumo)
    figura_tempo(resumo)
    figura_memoria(resumo)
    print(f"gravados {os.path.relpath(RESUMO, RAIZ)}, results/figs/tempo.png e results/figs/memoria.png")


if __name__ == "__main__":
    main()
