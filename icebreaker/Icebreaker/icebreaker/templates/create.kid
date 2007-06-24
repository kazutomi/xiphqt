<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#" py:extends="'master.kid'">
<head>
  <meta content="text/html; charset=utf-8" http-equiv="Content-Type" py:replace="''" />
  <title>Welcome to TurboGears</title>
</head>
<body>
<form xmlns:py="http://purl.org/kid/ns#" name="${name}" action="${action}" method="${method}" class="listform" py:attrs="form_attrs">
    <div py:for="field in hidden_fields" py:replace="field.display(value_for(field), **params_for(field))" />
        <ul py:attrs="list_attrs">
            <li py:for="i, field in enumerate(fields)" class="${i%2 and 'odd' or 'even'}">
                <label class="fieldlabel" for="${field.field_id}" py:content="field.label" />
                <span py:replace="field.display(value_for(field), **params_for(field))" />
                <span py:if="error_for(field)" class="fielderror" py:content="error_for(field)" />
                <span py:if="field.help_text" class="fieldhelp" py:content="field.help_text" />
            </li>
            <li py:content="submit.display(submit_text)" />
        </ul>
</form>
<div xmlns:py='http://purl.org/kid/ns#'>
    <script type="text/javascript">
    var ${optrans_name} = new OptionTransfer('${name}.${available.name}',
                                             '${name}.${selected.name}');
    ${optrans_name}.setAutoSort(true);
    ${optrans_name}.saveNewLeftOptions('${name}.available_new');
    ${optrans_name}.saveAddedLeftOptions('${name}.available_added');
    ${optrans_name}.saveRemovedLeftOptions('${name}.available_removed');
    ${optrans_name}.saveNewRightOptions('${name}.selected_new');
    ${optrans_name}.saveAddedRightOptions('${name}.selected_added');
    ${optrans_name}.saveRemovedRightOptions('${name}.selected_removed');
    </script>
    ${display_field_for(available_new)}
    ${display_field_for(available_added)}
    ${display_field_for(available_removed)}
    ${display_field_for(selected_new)}
    ${display_field_for(selected_added)}
    ${display_field_for(selected_removed)}
    <table align='left' width='100%' class='selectshuttle'>
      <thead>
        <tr>
          <th class='selectshuttle-left' py:content='title_available'>Left Options</th>
          <th class='selectshuttle-middle'></th>
          <th class='selectshuttle-right' py:content='title_selected'>Right Options</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td class='selectshuttle-left'>${display_field_for(available)}</td>
          <td class='selectshuttle-middle'>
            <input type="button"
                   name="btn_selected"
                   id="${optrans_name}_btn_selected"
                   py:attrs="value=btn_to_selected"
                   onclick="${optrans_name}.transferRight()"
            /><br /><br />
            <input type="button"
                   name="btn_all_selected"
                   id="${optrans_name}_btn_all_selected"
                   py:attrs="value=btn_all_selected"
                   onclick="${optrans_name}.transferAllRight()"
            /><br /><br />
            <input type="button"
                   name="btn_all_available"
                   id="${optrans_name}_btn_all_available"
                   py:attrs="value=btn_all_available"
                  onclick="${optrans_name}.transferAllLeft()"
            /><br /><br />
            <input type="button"
                   name="btn_available"
                   id="${optrans_name}_btn_available"
                   py:attrs="value=btn_to_available"
                   onclick="${optrans_name}.transferLeft()"
            />
          </td>
          <td class='selectshuttle-right'>${display_field_for(selected)}</td>
        </tr>
        <tr py:if='add_link is not None'>
          <td class='selectshuttle-addlink' colspan='3'>
            <a target="${target}" href="${add_link}">
                <span py:strip="1" py:if="add_image_src is not None">
                    <img src="${add_image_src}" border="0" />
                </span>
                ${add_text}
            </a>
          </td>
        </tr>
      </tbody>
    </table>
    <script type="text/javascript">
      addLoadEvent(${optrans_name}.init(${form_reference}))
    </script>
</div>
</body>
</html>
