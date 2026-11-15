# Validação da implementação

Compilação Release Linux x86_64 com C++17 / GCC 14 e JUCE 8.0.6: VST3 e Standalone gerados. A biblioteca VST3 exporta GetPluginFactory, ModuleEntry e ModuleExit; suas dependências dinâmicas foram encontradas no sistema.

CTest: **2/2 suítes aprovadas**.

- DSP: 10 verificações em 44,1, 48 e 96 kHz (30 cenários), alinhamento das escalas com as marcas impressas e monotonicidade dos medidores.
- Integração: ganho neutro, restauração dos três parâmetros, preservação do balanço estéreo, clipping, mono e silêncio; ganho de saída aplicado desde a primeira amostra; captura de transientes entre quadros e correspondência entre ganho aplicado e medidores.
- Interação: três faders em três tamanhos de janela (100%, 200%, 300%); clique sem salto, arraste normal/fino, roda normal/fina, teclado e gestos de automação balanceados. Edição numérica com ponto/vírgula, limites mínimo/máximo e rejeição de entrada inválida/não finita.
- Editor renderizado em 954 × 921, com inspeção visual da captura atual do teste, incluindo movimentação dos controles e medidores alimentados por sinal senoidal.

O arquivo original `assets/mv2-reference.png` mantém o SHA-256:

`5d0f314d11ef824dbea773b2ef262bd1e4399b159833468441c914881a7fbce1`

A reconstrução do fundo atrás de controles móveis é aproximada em memória, pois a referência fornecida é uma imagem plana, sem camadas. O PNG original não foi modificado.

Não foram realizados carregamento/audição em DAW, comparação sonora com o produto Waves ou testes em Windows/macOS. O DSP e o gerenciamento de presets são próprios; não há certificação de equivalência ao algoritmo proprietário. Veja README.md para arquitetura, constantes e comandos de compilação/teste.

## Preparação para Windows x64

Clang 19.1.7 (frontend MSVC), LLD 19, LLVM resource tools e xwin 0.10.0 foram instalados localmente. O arquivo de distribuição xwin foi validado contra o SHA-256 publicado pelo projeto. SDK JUCE nativo instalado com sucesso. A compilação cruzada foi interrompida antes de compilar o plugin: o CRT/Windows SDK necessário ainda não pôde ser baixado, pois `download.visualstudio.microsoft.com` retornou HTTP 403. A liberação do domínio está salva em rascunho e ainda requer aplicação nas configurações. Os scripts Windows/Clang e o workflow manual são preparações, não evidência de uma compilação Windows aprovada.
