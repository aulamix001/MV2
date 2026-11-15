# MV2 Recreation — C++ / JUCE

Plugin de dinâmica mono/estéreo em C++17 e JUCE 8.0.6, nos formatos VST3 e aplicativo Standalone. Usa `assets/mv2-reference.png` como arte incorporada ao binário. O arquivo original permanece intacto; controles, números e medidores são desenhados em tempo real sobre a arte, com os faders reaproveitados como sprites.

## Controles

- **Low Level:** 0 a 48 dB, ganho máximo da compressão ascendente de sinais baixos.
- **High Level:** −48 a 0 dB, limiar da compressão descendente; zero desativa essa seção.
- **Output:** −48 a +18 dB, ganho de saída.
- Arraste verticalmente os faders (0,25 dB/pixel, sem salto no clique), com resposta igual nos três controles e em qualquer tamanho de janela. Shift/Ctrl/Cmd ativa ajuste fino de 0,025 dB/pixel. A roda ajusta 0,5 dB por passo; com modificador, 0,1 dB. As setas ajustam 0,1 dB. Duplo clique no número permite digitação; Enter confirma e Escape cancela. Aceita ponto ou vírgula decimal e rejeita entradas inválidas. Duplo clique no trilho retorna a 0 dB.
- Os dois canais usam o mesmo detector para preservar o balanço estéreo.
- Medidores a 60 Hz de ganho ascendente/redução efetivamente aplicado e de pico de saída, com captura de transientes entre quadros, decaimento de 24 dB/s, picos retidos no processador e LEDs de clipping. Clique na área de saída para limpar picos e LEDs.
- Undo/Redo, reset, comparação A/B, Load/Save de presets `.mv2preset`, ajuda na barra superior. Os três parâmetros são automatizáveis pelo host e salvos na sessão.
- Seletor de tamanho **100%–400% na faixa inferior direita** e redimensionamento pelo **canto inferior direito**, com proporção fixa: 318 × 307 até 1272 × 1228. Tamanho inicial: 636 × 614.

## Fidelidade e escopo

A referência visual aprovada é preservada byte a byte. O recorte dos sprites e a reconstrução local em memória das áreas atrás dos faders com textura e cores interpoladas acontecem somente durante a renderização; não são edições do PNG. Os valores variam com o áudio e os parâmetros, portanto uma interface em uso não é uma captura estática da imagem.

As páginas do manual fornecidas descrevem compressão ascendente/descendente, versões mono/estéreo, faixas de controles, medidores e gerenciamento da barra. Elas não publicam o DSP original. Esta implementação é independente: detector de pico ligado entre canais, ataque de 5 ms, release de 150 ms, suavização independente dos ganhos de 10 ms e knee de 6 dB. A seção ascendente atua abaixo de −18 dBFS com inclinação de 0,5 e ganho limitado por Low; a descendente usa razão 2:1 acima de High. Esses valores são escolhas desta implementação, não especificações verificadas da Waves. Não se promete equivalência sonora ou de curvas ao plugin original.

Defaults dos três parâmetros: 0 dB. Não há limitador de saída: ganhos positivos podem ultrapassar 0 dBFS, indicado pelos LEDs. A saída continua em ponto flutuante, sem corte artificial. Os faders Low/High e os medidores seguem os pontos das escalas impressas da arte, com interpolação entre marcas; a escala de saída vai até −60 dB. O fader Output tem uma curva própria de −48 a +18 dB, já que a escala ao lado é a dos medidores dBFS. O ganho de saída salvo é aplicado já na primeira amostra ao iniciar, sem uma passagem inicial em ganho unitário. A/B é mantido enquanto o editor está aberto; o host salva o conjunto de parâmetros ativo. O gerenciamento de presets é próprio, sem compatibilidade com presets Waves. O botão Setup restaura os defaults; o logo da referência é apenas parte da imagem.

## Compilar em Linux

Instale CMake ≥ 3.22, compilador C++17, Ninja e bibliotecas de desenvolvimento:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config libasound2-dev \
  libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
  libfreetype-dev libfontconfig-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

O CMake obtém o JUCE fixado no commit `51a8a6d7aeae7326956d747737ccf1575e61e209`. Para reutilizar uma cópia local, passe `-DJUCE_SOURCE_DIR=/caminho/JUCE`. Verifique a licença do JUCE (AGPLv3 ou comercial) antes de distribuir seu produto.

Artefatos:

- `build/MV2Recreation_artefacts/Release/VST3/MV2 Recreation.vst3/`
- `build/MV2Recreation_artefacts/Release/Standalone/MV2 Recreation`

Copie o bundle VST3 para `~/.vst3/` para uso em uma DAW Linux. O binário Linux não carrega em uma DAW Windows/macOS: compile na plataforma de destino com o toolchain correspondente. Essas plataformas não foram testadas neste ambiente.

## Ambiente de nuvem atual

`scripts/build-cloud.sh` reutiliza o JUCE e as bibliotecas locais preparados fora do checkout em `/workspace/juce` e `/workspace/toolchain`. Não precisa de permissões de administrador. `scripts/test-cloud.sh` inicia um Xvfb temporário, executa os testes e encerra somente esse servidor. O comando acima com as dependências do sistema é o procedimento portátil para outra máquina.

Os testes de DSP são independentes de JUCE. Para executá-los sem construir o plugin:

```sh
cmake -S . -B build-dsp -DMV2_BUILD_PLUGIN=OFF
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

O teste de integração instancia o processador e verifica ganho neutro, restauração do estado, ligação estéreo, clipping, mono/silêncio, captura de transientes, correspondência dos medidores com o ganho aplicado e renderização do editor. Exercita os três faders em três tamanhos de janela: clique sem salto, arraste normal e fino, roda, teclado, edição numérica e início/fim dos gestos enviados ao host. Ele exige um servidor gráfico; em Linux sem desktop, use `xvfb-run -a ctest --test-dir build --output-on-failure`. A captura `editor-preview.png` é gerada na pasta de execução, sem tocar na imagem original. Não houve medição comparativa contra o plugin comercial nem audição em DAW.

## Windows 64 bits

O projeto inclui `scripts/build-windows-x64.ps1` para compilar nativamente com Visual Studio 2022 C++ Build Tools e CMake. Execute no PowerShell:

```powershell
./scripts/build-windows-x64.ps1
```

O script compila VST3 e Standalone x64, executa os testes e só gera `dist/MV2-Recreation-Windows-x64.zip` se todas as etapas concluírem com sucesso. Instale o bundle `.vst3` inteiro em `C:\Program Files\Common Files\VST3` e reescaneie os plugins na DAW.

O workflow `.github/workflows/windows-x64.yml` executa esse processo em `windows-2022` a cada push em `main` e também por **Run workflow**. Após compilação e testes aprovados, disponibiliza o ZIP nos artefatos da execução e cria uma branch exclusiva `builds/windows-x64-<run_id>-<attempt>` com o pacote em `dist/`. A branch principal mantém o código-fonte; cada pacote é associado ao commit que o gerou. A criação da branch requer que a política do repositório permita escrita para o `GITHUB_TOKEN`. O runtime C/C++ é ligado estaticamente.

A compilação cruzada está preparada em `scripts/build-windows-x64-cloud.sh`, com Clang 19/LLD e um SDK JUCE nativo. Ainda depende do CRT/Windows SDK oficial da Microsoft, preparado por `scripts/prepare-windows-sdk-cloud.sh`. A rede atual bloqueou `download.visualstudio.microsoft.com` com HTTP 403; a liberação desse domínio foi salva como rascunho nas configurações do ambiente, preservando os presets existentes. Nenhum binário Windows foi produzido ou testado nesta etapa. O caminho MinGW foi descartado porque JUCE 8.0.6 o rejeita explicitamente; não foi alterado o framework para contornar essa restrição.
