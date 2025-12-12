using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class TextAnimator : MonoBehaviour
{
  private TextMeshProUGUI tmpText;
  private Coroutine animationCoroutine;

  IEnumerator Animator(){
    // 文字の表示数を0に(テキストが表示されなくなる)
    tmpText.maxVisibleCharacters = 0;

    // テキストの文字数分ループ
    for (var i = 0; i < tmpText.text.Length; i++){
      // 一文字ごとに0.2秒待機
      yield return new WaitForSeconds(0.1f);

      // 文字の表示数を増やしていく
      tmpText.maxVisibleCharacters = i + 1;
    }
  }

  public void Restart(){
    Run();
  }

  public void Run()
  {
    Debug.Log(tmpText.text);
    if (animationCoroutine != null){
      StopCoroutine(animationCoroutine);
    }

    animationCoroutine = StartCoroutine(Animator());
  }

  // Start is called before the first frame update
  void Start(){
    tmpText = GetComponent<TextMeshProUGUI>();
  }

  // Update is called once per frame
  void Update(){
      
  }
}
